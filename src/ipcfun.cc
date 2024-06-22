#include <sys/mman.h>

#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

#include "support.hh"

using namespace std;

constexpr size_t total_iterations = 100000;
constexpr size_t system_call_at = total_iterations / 2;

constexpr size_t num_random_vectors = 32;
constexpr size_t random_vector_size = 16;

struct SamplePair
{
  IPCCounter::Reading pre, post;
};

class Workload
{
  // Computation task with branches: sorting a random vector
  array<array<char, random_vector_size>, num_random_vectors> data_to_sort {}, mutable_data_to_sort {};
  vector<size_t> indices_to_sort {};

  // Numerical computation: matrix multiplication
  vector<Eigen::Matrix<float, 7, 7>> matrices { 3 };

public:
  Workload()
  {
    for ( auto& vec : data_to_sort ) {
      for ( auto& val : vec ) {
        val = rand();
      }
    }

    indices_to_sort.resize( total_iterations );
    for ( auto& index : indices_to_sort ) {
      index = rand() % num_random_vectors;
    }

    matrices[1].Random();
    matrices[2].Random();
  }

  void do_branchy_computation( size_t i )
  {
    const auto index = indices_to_sort.at( i );
    mutable_data_to_sort[index] = data_to_sort[index];
    sort( mutable_data_to_sort[index].begin(), mutable_data_to_sort[index].end() );
  }

  void do_matrix_computation() { matrices[0] = matrices[1] * matrices[2]; }
};

void trivial_memory_copy()
{
  volatile char x = 42;
  volatile char y;
  y = x;
  x = y;
}

void usage_error( const span<char*>& args )
{
  cerr << "Usage: " << args[0] << " \"syscall\"/\"nosyscall\" \"branchy\"/\"matrix\"\n";
  throw runtime_error( "invalid usage" );
}

tuple<bool, bool> process_arguments( const auto& args )
{
  if ( args.size() != 3 ) {
    usage_error( args );
  }

  bool do_syscall;
  if ( args[1] == "syscall"sv ) {
    do_syscall = true;
  } else if ( args[1] == "nosyscall"sv ) {
    do_syscall = false;
  } else {
    usage_error( args );
  }

  bool branchy;
  if ( args[2] == "branchy"sv ) {
    branchy = true;
  } else if ( args[2] == "matrix"sv ) {
    branchy = false;
  } else {
    usage_error( args );
  }

  return tie( do_syscall, branchy );
}

int main( int argc, char* argv[] )
{
  ios::sync_with_stdio( false );

  // Parse arguments
  if ( argc <= 0 ) {
    abort();
  }
  auto args = span( argv, argc );
  auto [do_syscall, branchy] = process_arguments( args );

  // Open dummy file
  int fd = memfd_create( "dummy", 0 );
  if ( fd < 0 ) {
    throw runtime_error( "memfd_create" );
  }

  // Prevent CPU migration
  lock_to_CPU_zero();

  // Initialize compute "workload"
  Workload workload;

  // Initialize monitoring of IPC (instructions per cycle)
  IPCCounter perf;
  vector<SamplePair> samples( total_iterations );
  perf.start();

  // In each iteration, do computation or a system call
  for ( unsigned int i = 0; i < total_iterations; ++i ) {
    const auto sample = perf.read();
    samples.at( i ).pre = sample;
    if ( i > 0 ) {
      samples.at( i - 1 ).post = sample;
    }

    if ( i == system_call_at ) {
      if ( do_syscall ) { // do 1-byte pwrite system call in this iteration
        if ( 1 != CheckSystemCall( "pwrite", pwrite( fd, "x", 1, 0 ) ) ) {
          throw runtime_error( "short write" );
        }
      } else { // copy one byte in user space (without a syscall)
        trivial_memory_copy();
      }
    } else { // otherwise, do some computation
      if ( branchy ) {
        workload.do_branchy_computation( i );
      } else {
        workload.do_matrix_computation();
      }
    }
  }

  samples.back().post = perf.read(); // final sample

  // Print the recorded performance counter data
  const auto index_inst = samples.at( system_call_at ).post.instructions; // zero index = immediately after syscall
  const auto index_cycle = samples.at( system_call_at ).post.cycles;      // zero index = immediately after syscall
  for ( unsigned int i = 0; i < total_iterations - 1; ++i ) {
    if ( i == system_call_at ) {
      cout << "# ";
    }
    const auto& sample = samples.at( i );
    const auto relative_instruction_beginning = sample.pre.instructions - index_inst;
    const auto relative_instruction_ending = sample.post.instructions - index_inst;

    const auto relative_cycle_beginning = sample.pre.cycles - index_cycle;
    const auto relative_cycle_ending = sample.post.cycles - index_cycle;

    const auto instructions = sample.post.instructions - sample.pre.instructions;
    const auto cycles = sample.post.cycles - sample.pre.cycles;
    const double ipc = double( instructions ) / double( cycles );

    const auto inst_box_middle = ( relative_instruction_beginning + relative_instruction_ending ) / 2;
    const auto inst_box_width = relative_instruction_ending - relative_instruction_beginning;

    const auto cycle_box_middle = ( relative_cycle_beginning + relative_cycle_ending ) / 2;
    const auto cycle_box_width = relative_cycle_ending - relative_cycle_beginning;

    cout << inst_box_middle << " " << ipc << " " << inst_box_width << " ";
    cout << cycle_box_middle << " " << ipc << " " << cycle_box_width << "\n";
  }

  return EXIT_SUCCESS;
}

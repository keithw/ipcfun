#include <sys/mman.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
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

int main()
{
  ios::sync_with_stdio( false );

  // Open dummy file
  int fd = memfd_create( "dummy", 0 );
  if ( fd < 0 ) {
    throw runtime_error( "memfd_create" );
  }

  // Create "computation" task: sorting a random vector
  array<array<char, random_vector_size>, num_random_vectors> data_to_sort, mutable_data_to_sort;
  for ( auto& vec : data_to_sort ) {
    for ( auto& val : vec ) {
      val = rand();
    }
  }
  vector<size_t> indices_to_sort( total_iterations );
  for ( auto& index : indices_to_sort ) {
    index = rand() % num_random_vectors;
  }

  // Prevent CPU migration
  lock_to_CPU_zero();

  // Initialize IPC computation
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

    if ( i == system_call_at ) { // do 1-byte pwrite system call in this iteration
      char data_to_write = 42;
      if ( sizeof( data_to_write )
           != static_cast<size_t>(
             CheckSystemCall( "pwrite", pwrite( fd, &data_to_write, sizeof( data_to_write ), 0 ) ) ) ) {
        throw runtime_error( "short write" );
      }
    } else { // otherwise, do computation
      const auto index = indices_to_sort[i];
      mutable_data_to_sort[index] = data_to_sort[index];
      sort( mutable_data_to_sort[index].begin(), mutable_data_to_sort[index].end() );
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

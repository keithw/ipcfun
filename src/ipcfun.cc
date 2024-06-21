#include <sys/mman.h>

#include <Eigen/Dense>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "support.hh"

using namespace std;

constexpr unsigned int total_iterations = 100000;
constexpr unsigned int system_call_at = total_iterations / 2;

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

  // Initialize matrices for computation
  vector<Eigen::Matrix<float, 7, 7>> matrices( 3 );
  matrices[1].Random();
  matrices[2].Random();

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

    if ( i == system_call_at ) { // do system call in this iteration (not printed)
      CheckSystemCall( "pwrite", pwrite( fd, nullptr, 0, 0 ) );
    } else { // otherwise, do computation and print stats
      matrices[0] = matrices[1] * matrices[2];
    }
  }

  samples.back().post = perf.read(); // final sample

  // Print the recorded performance counter data
  const auto index = samples.at( system_call_at ).post.instructions; // zero index = immediately after syscall
  for ( unsigned int i = 0; i < total_iterations - 1; ++i ) {
    if ( i == system_call_at ) {
      cout << "# ";
    }
    const auto& sample = samples.at( i );
    const auto relative_instruction_beginning = sample.pre.instructions - index;
    const auto relative_instruction_ending = sample.post.instructions - index;
    const auto instructions = sample.post.instructions - sample.pre.instructions;
    const auto cycles = sample.post.cycles - sample.pre.cycles;
    const double ipc = double( instructions ) / double( cycles );

    const auto box_middle = ( relative_instruction_beginning + relative_instruction_ending ) / 2;
    const auto box_width = relative_instruction_ending - relative_instruction_beginning;

    cout << box_middle << " " << ipc << " " << box_width << "\n";
  }

  return EXIT_SUCCESS;
}

#include <sys/mman.h>

#include <Eigen/Dense>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "support.hh"

using namespace std;

constexpr unsigned int total_iterations = 4000;
constexpr unsigned int system_call_at = 2000;

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
  vector<IPCCounter::Reading> samples( total_iterations );
  perf.start();

  // Do computation before the system call
  for ( unsigned int i = 0; i < system_call_at; ++i ) {
    samples.at( i ) = perf.read();
    matrices[0] = matrices[1] * matrices[2];
  }

  // Make system call
  CheckSystemCall( "pwrite", pwrite( fd, nullptr, 0, 0 ) );

  // Do computation after the system call
  for ( unsigned int i = system_call_at; i < total_iterations; ++i ) {
    samples.at( i ) = perf.read();
    matrices[0] = matrices[1] * matrices[2];
  }

  // Print the recorded performance counter data
  for ( unsigned int i = 1; i < total_iterations; ++i ) {
    const auto& this_sample = samples.at( i );
    const auto& last_sample = samples.at( i - 1 );
    const auto& index_sample = samples.at( system_call_at );

    const long int cum_instructions = this_sample.instructions - index_sample.instructions;
    const auto instructions = this_sample.instructions - last_sample.instructions;
    const auto cycles = this_sample.cycles - last_sample.cycles;
    const double ipc = double( instructions ) / double( cycles );

    cout << cum_instructions << " " << ipc << "\n";
  }

  return EXIT_SUCCESS;
}

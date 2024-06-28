#include <sys/mman.h>

#include <Eigen/Dense>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

#include "support.hh"

using namespace std;

struct SamplePair
{
  uint64_t pre, post;
};

class Workload
{
  using T1 = Eigen::Matrix<float, 8, 8>;
  using T2 = Eigen::Matrix<float, 8, 5>;
  using T3 = Eigen::Matrix<float, 8, 5>;

  // Numerical computation: matrix multiplication
  unique_ptr<T1> matrix1 { make_unique<T1>() };
  unique_ptr<T2> matrix2 { make_unique<T2>() };
  unique_ptr<T3> matrix3 { make_unique<T3>() };

public:
  Workload()
  {
    matrix1->Random();
    matrix2->Random();
    matrix3->Random();
  }

  void do_matrix_computation() { *matrix3 = *matrix1 * *matrix2; }
};

void usage_error( span<char*> args )
{
  cerr << "Usage: " << args[0] << " interval\n";
  throw runtime_error( "invalid usage" );
}

int main( int argc, char* argv[] )
{
  ios::sync_with_stdio( false );

  // Parse arguments
  if ( argc <= 0 ) {
    abort();
  }
  auto args = span( argv, argc );
  if ( args.size() != 3 ) {
    usage_error( args );
  }
  auto total_iterations = to_uint64( args[1] );
  auto interval = to_uint64( args[2] );

  // Open dummy file
  int fd = memfd_create( "dummy", 0 );
  if ( fd < 0 ) {
    throw runtime_error( "memfd_create" );
  }

  // Initialize compute "workload"
  Workload workload;

  // Store TSC samples
  vector<SamplePair> samples( total_iterations );

  uint64_t syscall_count = 0;

  // In each iteration, do computation and record the TSC before and after.
  // Also, sometimes do a syscall at user-controlled interval (outside the pair of TSC samples).
  for ( size_t i = 0; i < total_iterations; ++i ) {
    samples.at( i ).pre = read_tsc();

    workload.do_matrix_computation();

    samples.at( i ).post = read_tsc();

    if ( i % interval == ( interval - 1 ) ) {
      if ( 0 != pwrite( fd, nullptr, 0, 0 ) ) {
        throw runtime_error( "pwrite returned error" );
      }
      syscall_count++;
    }
  }

  // Print the recorded performance counter data
  uint64_t total_tsc_in_user_code {};
  for ( const auto& sample : samples ) {
    total_tsc_in_user_code += ( sample.post - sample.pre );
  }

  /*
    To adjust TSC counts to IPC without using RDPMC, we need
    to know the number of TSC ticks per second and the number of
    instructions per iteration.

    With (8x8) x (8x5) matrix multiplication, running
       `sudo nice -n -20 perf stat ./build/src/ipcfun 100000000 10000000000`
       with the `performance` CPU governor on AMD Ryzen 7 PRO 4750U (turbo clock 4.1 GHz)

    Produces:
       # Total TSC ticks: 28562935220
       # Total TSC ticks in user code: 22014253224
       # Average TSC per iteration: 220.143
       # Average instructions per TSC tick: 10.4137
       # Executed 100000000 iterations, with 0 syscalls.
    [...]
     72,804,049,462      cycles
    124,607,089,513      instructions
    [...]
    17.884952007 seconds time elapsed
    16.986015000 seconds user

    Instructions per iteration = 124607089513 / 100000000 = 1246...
    TSC ticks per second = 28562935220 / (17.884952007 seconds) = 1597.0373 megahertz
    Cycles per TSC tick = 72804049462 / 28562935220 = 2.5489...
    Expected cycles per TSC tick = 4.1 GHz / 1.6 GHz = 2.5625...
  */
  static constexpr double instructions_per_iteration = 1246;
  static constexpr double cycles_per_tsc_tick = 2.548899;

  double average_tsc_per_iteration = double( total_tsc_in_user_code ) / double( total_iterations );
  double average_user_ipc = instructions_per_iteration / ( cycles_per_tsc_tick * average_tsc_per_iteration );

  cout << "# Total TSC ticks: " << samples.back().post - samples.front().pre << "\n";
  cout << "# Total TSC ticks in user code: " << total_tsc_in_user_code << "\n";
  cout << "# Average TSC per iteration: " << average_tsc_per_iteration << "\n";
  cout << "# Average user instructions per cycle: " << average_user_ipc << "\n";
  cout << "# Executed " << total_iterations << " iterations, with " << syscall_count << " syscalls.\n";
  cout << instructions_per_iteration * interval << " " << average_user_ipc << "\n";

  return EXIT_SUCCESS;
}

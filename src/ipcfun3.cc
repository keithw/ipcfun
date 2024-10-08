#include <sys/mman.h>

#include <Eigen/Dense>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <span>

#include "support.hh"

using namespace std;

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
  cerr << "Usage: " << args[0] << " total_iterations when_sycall [=\"at_end\" or \"interspersed\" or \"never\"]\n";
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
  auto when = args[2];
  bool syscalls_at_end, syscalls_interspersed;

  if ( when == "at_end"sv ) {
    syscalls_at_end = true;
    syscalls_interspersed = false;
  } else if ( when == "interspersed"sv ) {
    syscalls_at_end = false;
    syscalls_interspersed = true;
  } else if ( when == "never"sv ) {
    syscalls_at_end = false;
    syscalls_interspersed = false;
  } else {
    usage_error( args );
  }

  // Open dummy file
  int fd = memfd_create( "dummy", 0 );
  if ( fd < 0 ) {
    throw runtime_error( "memfd_create" );
  }

  // Initialize compute "workload"
  Workload workload;

  uint64_t syscall_count = 0;

  // In each iteration, do computation and record the TSC before and after.
  // Also, sometimes do a syscall at user-controlled interval (outside the pair of TSC samples).
  for ( size_t i = 0; i < total_iterations; ++i ) {
    workload.do_matrix_computation();

    if ( syscalls_interspersed ) {
      if ( 0 != pwrite( fd, nullptr, 0, 0 ) ) {
        throw runtime_error( "pwrite returned error" );
      }
      ++syscall_count;
    }
  }

  if ( syscalls_at_end ) {
    for ( size_t i = 0; i < total_iterations; ++i ) {
      if ( 0 != pwrite( fd, nullptr, 0, 0 ) ) {
        throw runtime_error( "pwrite returned error" );
      }
      ++syscall_count;
    }
  }

  cerr << "Iterations: " << total_iterations << "\n";
  cerr << "Syscall count: " << syscall_count << "\n";
  cerr << "Syscalls interspersed: " << syscalls_interspersed << "\n";
  cerr << "Syscalls all at the end: " << syscalls_at_end << "\n";

  return EXIT_SUCCESS;
}

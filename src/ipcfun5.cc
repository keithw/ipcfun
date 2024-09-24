#include <sys/mman.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

#include "support.hh"

using namespace std;

const int N = 10000000;
const int n_pages = 4;
#define page_range 10240
int a[page_range][1024];
char buffer[1024];

struct node
{
  int* addr;
  struct node* next;
} addr[page_range];

class Workload
{
public:
  Workload()
  {
    struct node* head = &addr[0];
    for ( int i = 0; i < n_pages; i++ ) {
      head->addr = &a[( rand() % page_range )][128];
      head->next = &addr[( rand() % page_range )];
      head = head->next;
    }
  }

  int do_computation()
  {
    volatile int tmp;
    struct node* head = &addr[0];
    for ( int j = 0; j < n_pages; j++ ) {
      tmp = *( head->addr );
      head = head->next;
    }
    return tmp;
  }
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
    workload.do_computation();

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

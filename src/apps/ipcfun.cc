#include <papi.h>
#include <sys/mman.h>

#include <Eigen/Dense>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

const char* str_or_null( const char* x )
{
  return x ? x : "(null)";
}

inline void CheckPAPICall( const char* attempt, int ret )
{
  if ( ret != PAPI_OK ) {
    throw runtime_error( string( attempt ) + ": " + str_or_null( PAPI_strerror( ret ) ) );
  }
}

using MyMatrix = Eigen::Matrix<double, 256, 256>;

inline float instructions_per_cycle()
{
  float real_time, proc_time, ipc;
  long long ins;

  CheckPAPICall( "PAPI_inc", PAPI_ipc( &real_time, &proc_time, &ins, &ipc ) );

  return ipc;
}

class tagged_error : public std::system_error
{
private:
  std::string attempt_and_error_;
  int error_code_;

public:
  tagged_error( const std::error_category& category, const std::string_view s_attempt, const int error_code )
    : system_error( error_code, category )
    , attempt_and_error_( std::string( s_attempt ) + ": " + std::system_error::what() )
    , error_code_( error_code )
  {}

  const char* what() const noexcept override { return attempt_and_error_.c_str(); }

  int error_code() const { return error_code_; }
};

int CheckSystemCall( const char* attempt, int ret )
{
  if ( ret < 0 ) {
    throw tagged_error( system_category(), attempt, errno );
  }
  return ret;
}

int main()
{
  ios::sync_with_stdio( false );

  // Open dummy file
  int fd = memfd_create( "dummy", 0 );
  if ( fd < 0 ) {
    throw runtime_error( "memfd_create" );
  }

  // Initialize matrices
  vector<MyMatrix> matrices( 3 );
  matrices[1].Random();
  matrices[2].Random();

  vector<float> ipc_measurements( 4000 );

  for ( int i = 0; i < 2000; ++i ) {
    ipc_measurements.at( i ) = instructions_per_cycle();
    matrices[0] = matrices[1] * matrices[2];
  }

  CheckSystemCall( "pwrite", pwrite( fd, nullptr, 0, 0 ) );

  for ( int i = 2000; i < 4000; ++i ) {
    ipc_measurements.at( i ) = instructions_per_cycle();
    matrices[0] = matrices[1] * matrices[2];
  }

  return EXIT_SUCCESS;
}

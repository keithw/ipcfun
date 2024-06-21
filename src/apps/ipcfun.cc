#include <papi.h>
#include <papiStdEventDefs.h>
#include <sys/mman.h>

#include <Eigen/Dense>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

using MyMatrix = Eigen::Matrix<double, 256, 256>;

const char* str_or_null( const char* x )
{
  return x ? x : "(null)";
}

class IPCCounter
{
  int event_set_;

  static inline int CheckPAPICall( const char* attempt, int ret )
  {
    if ( ret != PAPI_OK ) {
      throw runtime_error( string( attempt ) + ": " + str_or_null( PAPI_strerror( ret ) ) );
    }
    return ret;
  }

public:
  IPCCounter() : event_set_( PAPI_NULL )
  {
    const int version_or_err = PAPI_library_init( PAPI_VER_CURRENT );
    if ( version_or_err != PAPI_VER_CURRENT ) {
      CheckPAPICall( "PAPI_library_init", version_or_err );
    }
    CheckPAPICall( "PAPI_create_eventset", PAPI_create_eventset( &event_set_ ) );
    CheckPAPICall( "PAPI_add_event", PAPI_add_event( event_set_, PAPI_TOT_INS ) );
    CheckPAPICall( "PAPI_add_event", PAPI_add_event( event_set_, PAPI_TOT_CYC ) );
  }

  struct Reading
  {
    long long instructions;
    long long cycles;
  };

  Reading read()
  {
    Reading ret;
    CheckPAPICall( "PAPI_read", PAPI_read( event_set_, &ret.instructions ) );
    return ret;
  }

  void start() { CheckPAPICall( "PAPI_start", PAPI_start( event_set_ ) ); }
};

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

  // Initialize matrices for computation
  vector<MyMatrix> matrices( 3 );
  matrices[1].Random();
  matrices[2].Random();

  // Initialize IPC computation
  IPCCounter perf;

  vector<IPCCounter::Reading> ipc_measurements( 4000 );

  perf.start();

  for ( int i = 0; i < 2000; ++i ) {
    ipc_measurements.at( i ) = perf.read();
    matrices[0] = matrices[1] * matrices[2];
  }

  CheckSystemCall( "pwrite", pwrite( fd, nullptr, 0, 0 ) );

  for ( int i = 2000; i < 4000; ++i ) {
    ipc_measurements.at( i ) = perf.read();
    matrices[0] = matrices[1] * matrices[2];
  }

  for ( int i = 1; i < 4000; ++i ) {
    auto instructions = ipc_measurements.at( i ).instructions - ipc_measurements.at( i - 1 ).instructions;
    auto cycles = ipc_measurements.at( i ).cycles - ipc_measurements.at( i - 1 ).cycles;
    cout << i << " " << instructions << " " << cycles << " " << double( instructions ) / double( cycles ) << "\n";
  }

  return EXIT_SUCCESS;
}

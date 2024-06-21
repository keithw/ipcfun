#include "papi.h"
#include <stdexcept>

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

int main( void )
{
  float real_time, proc_time, ipc;
  long long ins;

  CheckPAPICall( "PAPI_inc", PAPI_ipc( &real_time, &proc_time, &ins, &ipc ) );
}

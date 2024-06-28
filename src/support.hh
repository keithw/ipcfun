#pragma once

#include <charconv>
#include <cstdint>
#include <papi.h>
#include <sched.h>
#include <string>
#include <system_error>
#include <x86intrin.h>

inline const char* str_or_null( const char* x )
{
  return x ? x : "(null)";
}

class IPCCounter
{
  int event_set_;

  static inline int CheckPAPICall( const char* attempt, int ret )
  {
    if ( ret != PAPI_OK ) {
      throw std::runtime_error( std::string( attempt ) + ": " + str_or_null( PAPI_strerror( ret ) ) );
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

inline int CheckSystemCall( const char* attempt, int ret )
{
  if ( ret < 0 ) {
    throw tagged_error( std::system_category(), attempt, errno );
  }
  return ret;
}

inline void lock_to_CPU_zero()
{
  cpu_set_t set;
  CPU_ZERO( &set );
  CPU_SET( 0, &set );
  CheckSystemCall( "sched_setaffinity", sched_setaffinity( getpid(), sizeof( set ), &set ) );
}

inline uint64_t to_uint64( std::string_view str )
{
  uint64_t ret = -1;
  const auto [ptr, ignore] = std::from_chars( str.data(), str.data() + str.size(), ret );
  if ( ptr != str.data() + str.size() ) {
    str.remove_prefix( ptr - str.data() );
    throw std::runtime_error( "could not parse as integer: " + std::string( str ) );
  }

  return ret;
}

inline uint64_t read_tsc()
{
  // seems to be Intel's recommended sequence of fences (https://www.felixcloutier.com/x86/rdtsc)
  _mm_mfence();
  _mm_lfence();
  uint64_t ret = __rdtsc();
  _mm_lfence();
  return ret;
}

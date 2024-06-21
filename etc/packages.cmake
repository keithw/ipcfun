find_package(PkgConfig REQUIRED)
pkg_check_modules(papi REQUIRED papi)
include_directories( ${papi_INCLUDE_DIRS} )
add_compile_options( ${papi_CFLAGS} )

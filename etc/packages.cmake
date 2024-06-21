find_package(PkgConfig REQUIRED)
pkg_check_modules(papi REQUIRED papi)
include_directories(${papi_INCLUDE_DIRS})
add_compile_options(${papi_CFLAGS})

pkg_check_modules(Eigen REQUIRED IMPORTED_TARGET GLOBAL eigen3)
include_directories(SYSTEM ${Eigen_INCLUDE_DIRS})
add_compile_options(${Eigen_CFLAGS})
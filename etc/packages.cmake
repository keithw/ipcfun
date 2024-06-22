find_package(PkgConfig REQUIRED)
pkg_check_modules(papi REQUIRED papi)
include_directories(${papi_INCLUDE_DIRS})

pkg_check_modules(Eigen REQUIRED IMPORTED_TARGET GLOBAL eigen3)
include_directories(SYSTEM ${Eigen_INCLUDE_DIRS})

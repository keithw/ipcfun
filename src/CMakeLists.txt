add_executable("ipcfun" "ipcfun.cc")
target_link_libraries(ipcfun ${papi_LDFLAGS} ${papi_LDFLAGS_OTHER})

add_executable("ipcfun2" "ipcfun2.cc")
target_link_libraries(ipcfun2)

add_executable("ipcfun3" "ipcfun3.cc")
target_link_libraries(ipcfun3)

add_executable("tscdemo" "tscdemo.cc")

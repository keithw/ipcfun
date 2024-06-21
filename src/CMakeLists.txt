add_executable("ipcfun" "ipcfun.cc")
target_link_libraries(ipcfun ${papi_LDFLAGS} ${papi_LDFLAGS_OTHER})

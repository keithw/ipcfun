add_executable("ipcfun" "ipcfun.cc")
target_link_libraries(ipcfun ${papi_LDFLAGS} ${papi_LDFLAGS_OTHER})

add_executable("ipcfun2" "ipcfun2.cc")
target_link_libraries(ipcfun2)

add_executable("ipcfun3" "ipcfun3.cc")
target_link_libraries(ipcfun3)

add_executable("ipcfun4" "ipcfun4.cc")
target_link_libraries(ipcfun4)

add_executable("ipcfun5" "ipcfun5.cc")
target_link_libraries(ipcfun5)

add_executable("tscdemo" "tscdemo.cc")

set (CMAKE_CXX_STANDARD 23)

# ask for more warnings from the compiler
add_compile_options(-Wall -Wpedantic -Werror -Wextra -Wshadow -Wcast-qual -Wformat=2)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Weffc++>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unqualified-std-cast-call>)

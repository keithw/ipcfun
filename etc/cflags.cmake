set (CMAKE_CXX_STANDARD 23)

# ask for more warnings from the compiler
add_compile_options(-Wall -Wpedantic -Werror -Wextra -Wshadow -Wcast-qual -Wformat=2 -DEIGEN_STACK_ALLOCATION_LIMIT=0 -DEIGEN_NO_MALLOC -mtune=native -march=native)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Weffc++>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unqualified-std-cast-call>)

# TODO replace cmake with something better

cmake_minimum_required (VERSION 3.16)
project (hammer)

add_subdirectory(src)
add_subdirectory(tests)

add_executable(hammer_tests tests_main.c)
target_link_libraries(hammer_tests hammertests hammerrt)

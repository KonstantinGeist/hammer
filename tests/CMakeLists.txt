file(GLOB HAMMER_TESTS_SRC "*.c")
add_library(libhammertests STATIC ${HAMMER_TESTS_SRC})

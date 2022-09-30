file(GLOB HAMMER_TESTS_SRC "*.c")
add_library(hammertests STATIC ${HAMMER_TESTS_SRC})

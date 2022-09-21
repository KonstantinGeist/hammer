./build_debug.sh
cd bin
valgrind --leak-check=full ./hammer_tests

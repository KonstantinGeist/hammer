cd bin
valgrind --leak-check=full ./hammer-tests "$@"
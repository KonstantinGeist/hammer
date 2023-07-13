cd bin
valgrind --leak-check=full --track-fds=yes ./hammer-tests "$@"
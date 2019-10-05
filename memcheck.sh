#!/bin/bash
echo "Parameters: $@" > memcheck.log
valgrind --tool=memcheck --leak-check=full --show-reachable=yes --track-origins=yes -v \
	dist/Debug/GNU-Linux/lottieconverter $@ 2> >(tee -a memcheck.log >&2)

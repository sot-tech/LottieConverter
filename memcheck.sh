#!/bin/bash
truncate -s 0 memcheck.log
valgrind --tool=memcheck --leak-check=full --show-reachable=yes --track-origins=yes \
	build/lottieconverter $@ 2> >(tee -a memcheck.log >&2)

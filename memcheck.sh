#!/bin/sh

valgrind --tool=memcheck --leak-check=full --show-reachable=yes --track-origins=yes \
	build/lottieconverter $@

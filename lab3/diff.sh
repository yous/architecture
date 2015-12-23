#!/usr/bin/env bash

make

command="diff -u <(./cs311sim -n "$@" sample_input/double_loop.o) \
  <(./lab3_test/cs311test -n "$@" sample_input/double_loop.o)"
echo $command

diff -I "^PC: .*" -u <(./cs311sim -n "$@" sample_input/double_loop.o) \
  <(./lab3_test/cs311test -n "$@" sample_input/double_loop.o)

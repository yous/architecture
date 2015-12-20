#!/usr/bin/env bash

gcc cache_output_format.c -o cs311cache

function res_print()
{
  if [ $? -eq 0 ]; then
    echo -e '\x1b[32mPass\x1b[0m'
  else
    echo -e '\x1b[31mFail\x1b[0m'
  fi
}

for FILENAME in lab4_test/sample_* real_workload/*
do
  echo "Testing with ${FILENAME} ..."

  for CAPACITY in {2..13}
  do
    for WAY in {0..4}
    do
      for BLOCK in {2..5}
      do
        if (($CAPACITY >= $WAY + $BLOCK)); then
          option="$((1 << $CAPACITY)):$((1 << $WAY)):$((1 << $BLOCK))"
          echo -n -e "-c $option -x: \t"
          diff -u <(./cs311cache -c $option -x $FILENAME) <(./lab4_test/cs311cache -c $option -x $FILENAME) > /dev/null
          res_print
        fi
      done
    done
  done
done

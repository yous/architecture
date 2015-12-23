#!/usr/bin/env bash

make

function res_print()
{
  if [ $? -eq 0 ]; then
    echo -n -e '\x1b[32mPass\x1b[0m '
  else
    echo -n -e '\x1b[31mFail\x1b[0m '
  fi
}

for FILENAME in sample_input/*.o
do
  echo -e "Testing with ${FILENAME} ..."
  for OPT1 in "-p " ""
  do
    for OPT2 in "-d " ""
    do
      for OPT3 in "-nobp " ""
      do
        for OPT4 in "-f " ""
        do
          for OPT5 in "-n 40 " ""
          do
            for OPT6 in "-m 0x10000000:0x10010000 " ""
            do
              option=${OPT1}${OPT2}${OPT3}${OPT4}${OPT5}${OPT6}
              diff -I "^PC: .*" -u \
                <(./cs311sim ${option} $FILENAME) \
                <(./lab3_test/cs311test ${option} $FILENAME) >/dev/null
              res_print
              echo -e "Option: ${OPT1}${OPT2}${OPT3}${OPT4}${OPT5}${OPT6}"
            done
          done
        done
      done
    done
  done
  echo
done

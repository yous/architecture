#!/usr/bin/env bash

make

function res_print()
{
  if [ $? -eq 0 ]; then
    echo 'Pass'
  else
    echo 'Fail'
  fi
}

for FILENAME in sample_input/*.o
do
  echo "Testing with ${FILENAME} ..."

  echo -n -e "No option: \t\t\t\t"
  diff -u <(./cs311test $FILENAME) <(./cs311em $FILENAME) > /dev/null
  res_print

  echo -n -e "-m 0x400000:0x400100 -n 0: \t\t"
  diff -u <(./cs311test -m 0x400000:0x400100 -n 0 $FILENAME) \
    <(./cs311em -m 0x400000:0x400100 -n 0 $FILENAME) > /dev/null
  res_print

  echo -n -e "-m 0x10000000:0x10000100 -n 0: \t\t"
  diff -u <(./cs311test -m 0x10000000:0x10000100 -n 0 $FILENAME) \
    <(./cs311em -m 0x10000000:0x10000100 -n 0 $FILENAME) > /dev/null
  res_print

  echo -n -e "-d -m 0x400000:0x400100 -n 0: \t\t"
  diff -u <(./cs311test -d -m 0x400000:0x400100 -n 0 $FILENAME) \
    <(./cs311em -d -m 0x400000:0x400100 -n 0 $FILENAME) > /dev/null
  res_print

  echo -n -e "-d -m 0x10000000:0x10000100 -n 0: \t"
  diff -u <(./cs311test -d -m 0x10000000:0x10000100 -n 0 $FILENAME) \
    <(./cs311em -d -m 0x10000000:0x10000100 -n 0 $FILENAME) > /dev/null
  res_print

  echo -n -e "-d -m 0x400000:0x400100 -n 1: \t\t"
  diff -u <(./cs311test -d -m 0x400000:0x400100 -n 1 $FILENAME) \
    <(./cs311em -d -m 0x400000:0x400100 -n 1 $FILENAME) > /dev/null
  res_print

  echo -n -e "-d -m 0x10000000:0x10000100 -n 1: \t"
  diff -u <(./cs311test -d -m 0x10000000:0x10000100 -n 1 $FILENAME) \
    <(./cs311em -d -m 0x10000000:0x10000100 -n 1 $FILENAME) > /dev/null
  res_print

  echo -n -e "-d: \t\t\t\t\t"
  diff -u <(./cs311test -d $FILENAME) <(./cs311em -d $FILENAME) > /dev/null
  res_print

  echo -n -e "-d -m 0x100000:0x100100: \t\t"
  diff -u <(./cs311test -d -m 0x100000:0x100100 $FILENAME) \
    <(./cs311em -d -m 0x100000:0x100100 $FILENAME) > /dev/null
  res_print

  echo -n -e "-m 0x400000:0x400100: \t\t\t"
  diff -u <(./cs311test -m 0x400000:0x400100 $FILENAME) \
    <(./cs311em -m 0x400000:0x400100 $FILENAME) > /dev/null
  res_print

  echo -n -e "-m 0x10000000:0x10000100: \t\t"
  diff -u <(./cs311test -m 0x10000000:0x10000100 $FILENAME) \
    <(./cs311em -m 0x10000000:0x10000100 $FILENAME) > /dev/null
  res_print

  echo -n -e "-d -m 0x400000:0x400100: \t\t"
  diff -u <(./cs311test -d -m 0x400000:0x400100 $FILENAME) \
    <(./cs311em -d -m 0x400000:0x400100 $FILENAME) > /dev/null
  res_print

  echo -n -e "-d -m 0x10000000:0x10000100: \t\t"
  diff -u <(./cs311test -d -m 0x10000000:0x10000100 $FILENAME) \
    <(./cs311em -d -m 0x10000000:0x10000100 $FILENAME) > /dev/null
  res_print
  echo
done

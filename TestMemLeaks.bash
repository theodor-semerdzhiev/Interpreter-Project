#!bin/bash
make clean
make
file=$1
leaks -atExit -- ./main.out $file
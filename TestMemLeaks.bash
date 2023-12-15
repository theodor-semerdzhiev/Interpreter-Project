#!bin/bash
make clean
make
leaks -atExit -- ./main.out
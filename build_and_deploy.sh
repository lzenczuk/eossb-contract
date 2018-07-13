#!/bin/bash

rm eossportbook.abi
eosiocpp -g eossportbook.abi eossportbook.cpp

cd ~/Documents/eos/build/
make

cleos set contract esbcontrac1 contracts/eossportbook -p esbcontrac1


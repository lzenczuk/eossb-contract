#!/bin/bash

rm eossportbook.abi
eosiocpp -g eossportbook.abi eossportbook.cpp

cd ~/Documents/eos/build/
make

cleos set contract eossportbook contracts/eossportbook -p eossportbook


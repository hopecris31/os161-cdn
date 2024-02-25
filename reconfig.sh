#!/bin/bash
cd src
./configure --ostree=$PWD/../root
bmake 
bmake install
cd kern/conf
./config PROJ3
cd ..
cd compile/PROJ3
bmake depend 
bmake
bmake install

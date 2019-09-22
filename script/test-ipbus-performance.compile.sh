#!/bin/bash

IPBUSFLAGS="-I ../../ipbus -L ../../ipbus/${CMTCONFIG} -lipbus"
ROOTFLAGS=`root-config --cflags --libs`

cmd="g++ ${IPBUSFLAGS} ${ROOTFLAGS} -o test-ipbus-performance test-ipbus-performance.cpp"
echo $cmd
eval $cmd


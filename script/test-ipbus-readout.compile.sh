#!/bin/bash

IPBUSFLAGS="-I ../../ipbus -L ../../ipbus/${CMTCONFIG} -lipbus"
ROOTFLAGS=`root-config --cflags --libs`
CPPFLAGS="-std=c++11"

cmd="g++ ${IPBUSFLAGS} ${ROOTFLAGS} ${CPPFLAGS} -o  test-ipbus-readout  test-ipbus-readout.cpp"
echo $cmd
eval $cmd


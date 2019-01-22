#! /bin/bash

cd ../../../..
./waf --run "mixed-ddos --RngRun=4 --frequency=100 --capacity=1500 --tolerance=500 --withStrategy=true --topo=fake-interest-ddos --output=mixed-f100-c1500-t500" 2>&1 | tee src/ndnSIM/Results/legitimate-percentage/mixed-f100-c1500-t500
cd -
Rscript mixed-ddos.R mixed-f100-c1500-t500 500 1500

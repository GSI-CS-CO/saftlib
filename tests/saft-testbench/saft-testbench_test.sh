#!/bin/bash
# Synopsis: Simply run saft-testbench, basically a small self-test.

# Source
source common.sh

stop_saft_software_tr
sudo saft-testbench


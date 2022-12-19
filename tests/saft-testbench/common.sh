#!/bin/bash

# Special options
set -xe

# Exports
export eb_id_string="current_eb_device_id_string.txt"
export eb_id="current_eb_device_id.txt"
export eb_id_clean="current_eb_device_id_clean.txt"
export sstr_log="saft-software-tr.txt"
export tr_default="tr0"

# Functions
function start_saft_software_tr()
{
  # Remove old logging files
  rm *.txt || true
  # Clean up
  stop_saft_software_tr
  # Start saft-software-tr and get the current eb device
  saft-software-tr >> $sstr_log &
  sleep 10
  cat $sstr_log | grep "eb-device: /dev/pts/" > $eb_id_string
  # Remove "eb-device:" from file
  cat $eb_id_string | awk '{print $2}' > $eb_id
  # Remove leading "/" from file, otherwise saftd will fail
  sed '1s/^.//' $eb_id > $eb_id_clean
  # Finally start saftd
  sudo saftd $tr_default:$(cat $eb_id_clean)
  sleep 1
}

function stop_saft_software_tr()
{
  # Just kill all spawned processes
  sudo killall saft-testbench || true
  sudo killall saft-software-tr || true
  sudo killall saftd || true
  sudo killall saft-ctl || true
  sleep 1
}


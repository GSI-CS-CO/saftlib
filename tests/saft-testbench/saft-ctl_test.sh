#!/bin/bash
# Syopsis: Small test of saft-ctl and saft-dm. 

# Source
source common.sh

# Exports
export snoop_file=snoop_file.txt
export random_events=random_events.txt
export snoop_compare_file=snoop_compare_file.txt
export random_events_compare_file=random_events_compare_file.txt

# Start
rm *.txt || true
start_saft_software_tr

# Get information from TR, we don't check the output here
saft-ctl $tr_default -i
saft-ctl $tr_default -s

# Generate a random schedule
i=1
while [[ $i -le 100 ]]; do
  event_id=$(tr -dc 'a-f0-9' < /dev/urandom | head -c16)
  parameter=$(tr -dc 'a-f0-9' < /dev/urandom | head -c16)
  time=$((i*100000))
  echo "0x$event_id 0x$parameter $time" >> $random_events
  echo "0x$event_id 0x$parameter" >> $random_events_compare_file
  ((i++))
done

# Snoop and inject (using saft-ctl only)
saft-ctl $tr_default snoop 0 0 0 -x > $snoop_file &
sleep 1
while read p; do
  saft-ctl $tr_default inject $p
done <$random_events
sleep 1
sudo killall saft-ctl

# Create comparable "snooped events" file
cat $snoop_file | awk '{print $4" "$6}' >> $snoop_compare_file

# Compare injected vs snooped events 
diff $random_events_compare_file $snoop_compare_file

# Snoop and inject (using saft-dm and saft-ctl)
rm $snoop_file 
saft-ctl $tr_default snoop 0 0 0 -x > $snoop_file &
saft-dm $tr_default -n 1 -p $random_events
sleep 10
sudo killall saft-ctl

# Create comparable "snooped events" file
rm $snoop_compare_file
cat $snoop_file | awk '{print $4" "$6}' >> $snoop_compare_file

# Compare injected vs snooped events 
diff $random_events_compare_file $snoop_compare_file

# Stop
stop_saft_software_tr

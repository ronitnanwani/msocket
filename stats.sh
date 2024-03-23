#!/bin/bash

# This script will print the the statistics to stats.txt

#  Array of p values
p_values=(0.35 0.4 0.45 0.5)

# Loop through the p values
for p in "${p_values[@]}"
do

    # Run initmsocket with p and should see the same directory as the script
    ./initmsocket $p &

    sleep 2
    # Run user2 with 6001 and 6000
    ./user2 6001 6000 &

    # Run user1 with 6000 and 6001 and wait until it finishes and then sleep for 20 seconds and then send sigint to the initmsocket and user2 and delete 6000r.txt and 6001s.txt
    ./user1 6000 6001
    sleep 50
    pkill -2 user2
    sleep 2
    pkill -2 initmsocket
    rm 6000r.txt
    

done


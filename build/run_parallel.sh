#!/bin/bash

NPROC=32
TOTAL_POINTS=1002001

CHUNK=$(( (TOTAL_POINTS + NPROC - 1) / NPROC ))

for ((i=0; i<NPROC; i++))
do
    START=$((i * CHUNK))
    END=$(((i + 1) * CHUNK - 1))

    if [ $END -ge 1002000 ]; then
        END=1002000
    fi

    screen -dmS scan_$i bash -c "
        ./crack run3.mac $START $END > log_${START}_${END}.txt 2>&1
    "

    echo "Started: $START -> $END"
done
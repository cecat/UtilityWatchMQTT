#!/usr/bin/env bash
#
# set_danger.sh - get or set the DANGER threshold on the Photon at runtime
#
# Usage:
#   ./set_danger.sh        - print current DANGER value
#   ./set_danger.sh <n>    - update DANGER to <n>
#
# To find your device name: particle list

DEVICE="cat_utils"

if [ -z "$1" ]; then
    current=$(particle get "$DEVICE" danger 2>/dev/null)
    echo "Danger is set to ${current}."
    echo "Use $0 <n> to update."
else
    result=$(particle call "$DEVICE" setDanger "$1" 2>/dev/null)
    if [ "$result" = "-1" ]; then
        echo "Error: invalid value '$1'. Please provide a positive integer."
    else
        echo "Danger updated to ${result}."
    fi
fi

#!/bin/bash

# argument number error
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <File name> <Title number> <Description>"
    exit 1
fi

# file exist error
if [ ! -f "$1" ]; then
    echo "Error: File '$1' not found."
    exit 1
fi

OS='Ubuntu 20.04.6 LTS 64bits'
Author='Kim Tae Wan'
StudentID='2020202034'

cat <<EOF | cat - "$1" > temp && mv temp "$1"
///////////////////////////////////////////////////////////////////////
// File Name     : %-49s //
// Date          : %-47s //
// OS            : %-47s //
// Author        : %-47s //
// Student ID    : %-47s //
// ----------------------------------------------------------------- //
// Title         : %-47s //
// Description   : %-47s                                             //
///////////////////////////////////////////////////////////////////////
EOF

# use printf
printf "$(basename "$1")" "$(date +'%Y/%m/%d')" "$(OS)" "$(Author)" "$(StudentID)" "$2" "$3" >> "$1"

echo "Header added to $1 successfully."


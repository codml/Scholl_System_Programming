#!/bin/bash

# argument number error
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <File name> <Title number>"
    exit 1
fi

# file exist error
if [ ! -f "$1" ]; then
    touch $1
fi

# copy to temp, add header
cat $1 > temp
rm $1
echo -e "////////////////////////////////////////////////////////////////////////" > $1
printf "// File Name	:%-52s //\n" $1 >> $1
printf "// Date		:%d/%d/%-45d //\n" $(date +%Y) $(date +%m) $(date +%d) >> $1
printf "// OS		:%-52s //\n" 'Ubuntu 20.04.6 LTS 64bits' >> $1
#change by your name
printf "// Author	:%-52s //\n" 'Kim Tae Wan' >> $1
#change by your ID
printf "// Student ID	:%-52d //\n" '2020202034' >> $1
echo "// ------------------------------------------------------------------ //" >> $1
printf "// Title	:System Programming Assignment #%-21s //\n" $2 >> $1
printf "// Description	:%-52s //\n" '...' >> $1
echo -e "////////////////////////////////////////////////////////////////////////\n" >> $1

cat temp >> $1
rm temp

echo "Header added to $1 successfully."


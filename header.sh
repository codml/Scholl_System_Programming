#!/bin/bash

# argument number error
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <File name> <#-#> <subject> <function #>" 
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
printf "// File Name    :%-52s //\n" $1 >> $1
printf "// Date         :%s/%s/%s%42s //\n" $(date +%Y) $(date +%m) $(date +%d) "" >> $1
printf "// OS           :%-52s //\n" 'Ubuntu 20.04.6 LTS 64bits' >> $1
#change by your name
printf "// Author       :%-52s //\n" 'Kim Tae Wan' >> $1
#change by your ID
printf "// Student ID   :%-52d //\n" '2020202034' >> $1
echo "// ------------------------------------------------------------------ //" >> $1
printf "// Title        :System Programming Assignment #%s: %-17s//\n" $2 $3 >> $1
printf "// Description  :%-52s //\n" '...' >> $1
echo -e "////////////////////////////////////////////////////////////////////////\n\n" >> $1

# function header
for i in $(seq $4)
do
	echo -e "////////////////////////////////////////////////////////////////////////" >> $1
	echo -e "// Function Name                                                      //" >> $1
	echo -e "// ================================================================== //" >> $1
	echo -e "// Input: type -> comments                                            //" >> $1
	echo -e "//                                                                    //" >> $1
	echo -e "//                                                                    //" >> $1
	echo -e "//                                                                    //" >> $1
	echo -e "// Output: type    value comments                                     //" >> $1
	echo -e "//                                                                    //" >> $1
	echo -e "//                                                                    //" >> $1
	echo -e "// Purpose: comments                                                  //" >> $1
	echo -e "////////////////////////////////////////////////////////////////////////\n\n" >> $1
done

#restore original code
cat temp >> $1
rm temp

echo "Header added to $1 successfully."


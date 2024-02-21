#!/bin/bash

if (( $# !=2 )); then
    echo "usage: bash netasciify.sh input_filename output_filename"
    exit 1
fi

if [ $1 == $2 ]; then
    echo "input_filename and output_filename cannot be the same."
    exit 1
fi

cp $1 $2.tmp1
touch $2.tmp2

for i in {1..6..1}
	do
	    octal=$(echo "obase=8; $i" | bc)
	    tr -d  \\$octal < $2.tmp1 | tee -a $2.tmp2
	    mv $2.tmp2 $2.tmp1
	    touch $2.tmp2
	done

for i in {14..31..1}
	do
	    octal=$(echo "obase=8; $i" | bc)
	    tr -d \\$octal < $2.tmp1 | tee -a $2.tmp2
	    mv $2.tmp2 $2.tmp1
	    touch $2.tmp2
	done
	
for i in {127..255..1}
	do
	    octal=$(echo "obase=8; $i" | bc)
	    echo "Number: $octal"
	    tr -d \\$octal < $2.tmp1 | tee -a $2.tmp2
	    mv $2.tmp2 $2.tmp1
	    touch $2.tmp2
	done

mv $2.tmp1 $2
rm $2.tmp2

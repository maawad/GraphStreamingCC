#! /bin/bash

# The specific test we want to run
executable=$1

# for every input file specified on the command line
for input in "${@:2}"
do
	echo ""
	echo "input file: $input"
	
	mkdir test_`basename $input`_data
	ln -sf $input test/res/current_test.stream
        
	echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	echo "num_groups=40"              >> streaming.conf
	echo "group_size=1"               >> streaming.conf
	"$executable"			
	mv runtime_data.txt test_`basename $input`_data/g40_s1_data.txt

	echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	echo "num_groups=20"              >> streaming.conf
	echo "group_size=2"               >> streaming.conf
	"$executable"			
	mv runtime_data.txt test_`basename $input`_data/g20_s2_data.txt

	echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	echo "num_groups=10"              >> streaming.conf
	echo "group_size=4"               >> streaming.conf
	"$executable"			
	mv runtime_data.txt test_`basename $input`_data/g10_s4_data.txt
	
	echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	echo "num_groups=8"               >> streaming.conf
	echo "group_size=5"               >> streaming.conf
	"$executable"			
	mv runtime_data.txt test_`basename $input`_data/g8_s5_data.txt

	echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	echo "num_groups=4"               >> streaming.conf
	echo "group_size=10"              >> streaming.conf
	"$executable"			
	mv runtime_data.txt test_`basename $input`_data/g4_s10_data.txt

	echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	echo "num_groups=2"               >> streaming.conf
	echo "group_size=20"              >> streaming.conf
	"$executable"			
	mv runtime_data.txt test_`basename $input`_data/g2_s20_data.txt
	
	echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	echo "num_groups=1"               >> streaming.conf
	echo "group_size=40"              >> streaming.conf
	"$executable"			
	mv runtime_data.txt test_`basename $input`_data/g1_s40_data.txt
done

# delete buffer tree data if it exists
rm BUFFTREEDATA/*



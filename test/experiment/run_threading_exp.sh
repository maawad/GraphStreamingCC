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
        
	#echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	#echo "num_groups=1"               >> streaming.conf
	#echo "group_size=1"               >> streaming.conf
	#"$executable"			
	#mv runtime_data.txt test_`basename $input`_data/g1_s1_data.txt

	for (( g=4 ; g <= 44; g+=4 ))
	do
		echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
		echo "num_groups=$((g))"          >> streaming.conf
		echo "group_size=1"               >> streaming.conf
		"$executable"			
		mv runtime_data.txt test_`basename $input`_data/g$((g))_s1_data.txt
		sleep 30
	done	
	echo "path_prefix=./BUFFTREEDATA/" > streaming.conf
	echo "num_groups=46"              >> streaming.conf
	echo "group_size=1"               >> streaming.conf
	"$executable"			
	mv runtime_data.txt test_`basename $input`_data/g46_s1_data.txt

done

# delete buffer tree data if it exists
rm BUFFTREEDATA/*



#! /bin/bash

# run the experiments as follows
# test/experiment/run_experiments [executable] [input_files]
# executable = ./experiment_memory or ./experiment_external
# input_files = the graph streams to test upon

# The specific test we want to run
executable=$1

# for every input file specified on the command line
mkdir ./results

for input in "${@:2}"
do
	echo "input file: $input"

	mkdir results/test_`basename $input`

	# either experiment_memory or experiment_external
	$executable $input results/test_`basename $input`/runtime_data.txt & ./track.sh $! > results/test_`basename $input`/profiling_data.txt
done

# delete buffer tree data if it exists
# rm BUFFTREEDATA/*

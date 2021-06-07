#! /bin/bash

# run the experiments as follows
# test/experiment/run_experiments [executable] [input_files]
# executable = ./experiment_memory or ./experiment_external
# input_files = the graph streams to test upon

# The specific test we want to run
executable=$1

# for every input file specified on the command line
for input in "${@:2}"
do
	echo "input file: $input"

	# TODO: put the code for running the memory profiler here
	# & top ... | awk ... 
	# TODO: pipe the above output to a file
	
	mkdir test_`basename $input`
	ln -sf $input test/res/current_test.stream
	
	"$executable" # either experiment_memory or experiment_external
	
	mv runtime_data.txt test_`basename $input`/
	# TODO: copy the awk output over as well.
done

# delete buffer tree data if it exists
rm BUFFTREEDATA/*

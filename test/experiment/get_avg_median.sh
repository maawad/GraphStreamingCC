
for file in "$@"
do
	echo $file
	
	# print the last line of the file which has the average runtime information
	echo average
	tail -n 1 $file
	
	# Remove the last 2 lines from the file
	sed "$(( $(wc -l <$file)-1 )),$ d" $file > /tmp/file_trunc
	
	#calculate the median point of the file
	med=$(( $(wc -l <$file)/2 ))
	
	# sort the file
	sort -g -k 2 /tmp/file_trunc > /tmp/file_sorted
	
	# print the median line
	echo median
	sed -n $((med))p /tmp/file_sorted

	# cleanup
	rm /tmp/file_trunc
	rm /tmp/file_sorted
done

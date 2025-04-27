#!/bin/bash
server_binary="../cpp-tftp-server-debug-build";

if (( $# !=1 )); then
    echo "usage: bash send num_simulated_connections"
    exit 1
fi

num_simulated_connections=$1

echo "Running test send_concurrent.sh with $num_simulated_connections concurrent connections."
echo "..."

echo "Creating test dirs to store the sent TFTP files."
echo "..."

for i in $( eval echo {1..$num_simulated_connections..1} )
	do
		mkdir test_dir_$i
	done

echo "Starting TFTP server."
echo "..."

#Track if error logs change so we can tell the user of this script
touch error_logs.txt
error_logs_status=$(md5sum error_logs.txt)
$server_binary --tftp-root ../test_transfers >> logs.txt 2>>error_logs.txt &
server_pid=$!

echo "Sending the file to the client over TFTP in each of the test dirs."
echo "..."

for i in $( eval echo {1..$num_simulated_connections..1} )
	do
		cd test_dir_$i
		#NOTE: requires "busybox" package to be installed.
		busybox tftp -gr beejs_guide.html localhost 3490 > /dev/null 2>&1 &
		pids[${i}]=$!
		cd ..
	done

for pid in ${pids[*]}; do
    wait $pid
done

echo "Finished TFTP transfers. Shutting down server."
echo "..."

kill -9 $server_pid
wait $server_pid 2>/dev/null

if [[ $error_logs_status != $(md5sum error_logs.txt) ]]; then
	echo "WARNING: the server sent output to standard error."
	echo "Check error_logs.txt for more details."
	echo "..."
fi

differences=""

echo "Checking for differences between original file and the transferred file(s)."
echo "..."

for i in $( eval echo {1..$num_simulated_connections..1} )
	do
		cd test_dir_$i
		#In case the TFTP transfer fails entirely this ensures the empty file is there to diff with
		touch beejs_guide.html
		differences+=$(diff ../../test_transfers/beejs_guide.html beejs_guide.html)
		cd ..
	done

echo "Removing test dirs."
echo "..."

for i in $( eval echo {1..$num_simulated_connections..1} )
	do
		rm -r test_dir_$i
	done
	
if [[ -z $differences ]]; then
	echo "No differences detected between the transferred file(s) and the original file."
	echo "..."
	echo "Test send_concurrent.sh passed with $num_simulated_connections concurrent connection(s)."
else
	echo $differences
	echo "..."
	echo "Differences were detected between the transferred file(s) and the original file."
	echo "This could either have been a bug in the way the server read the files,"
	echo "or it could have been due to connection timeouts."
	echo "..."
	echo "Test send_concurrent.sh failed with $num_simulated_connections concurrent connection(s)."
fi

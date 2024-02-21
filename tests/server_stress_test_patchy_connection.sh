#!/bin/bash
#requires busybox as a dependency
server_binary="../cpp-tftp-server-testing-build --drop 0.1 --duplicate 0.1";

if (( $# !=1 )); then
    echo "usage: bash server_stress_test_patchy_connection num_simulated_connections"
    exit 1
fi

num_simulated_connections=$1

echo "Running server stress test (patchy connection) with $num_simulated_connections concurrent (patchy) connections."
echo "..."

echo "Creating test dirs to store the received TFTP files."
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

echo "Requesting the files from the server over TFTP in each of the test dirs."
echo "..."

for i in $( eval echo {1..$num_simulated_connections..1} )
	do
		cd test_dir_$i
		#printf "mode octet\nget ../test_transfers/beejs_guide.html\nquit\n" | tftp localhost 3490
		busybox tftp -gr beejs_guide.html localhost 3490 > /dev/null 2>&1 &
		pids[${i}]=$!
		cd ..
	done

for pid in ${pids[*]}; do
    wait $pid
done

echo "Finished TFTP transfers. Shutting down server."
echo "..."

kill -9 $server_pid > /dev/null 2>&1
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
		differences+=$(diff ../../test_transfers/beejs_guide.html beejs_guide.html -q)
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
	echo "Server stress test (patchy connection) passed with $num_simulated_connections concurrent (patchy) connection(s)."
else
	echo "Differences were detected between the transferred file(s) and the original file."
	echo "This could either have been a bug in the way the server read the files,"
	echo "or it could have been due to connection timeouts."
	echo "..."
	echo "NOTE: since this test deliberately messes with packet inputs,"
	echo "this test failing probably is not a bug at all."
	echo "..."
	echo "Server stress test (patchy connection) failed with $num_simulated_connections concurrent (patchy) connection(s)."
fi

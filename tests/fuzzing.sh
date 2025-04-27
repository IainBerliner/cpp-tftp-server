#!/bin/bash
#requires busybox as a dependency
server_binary="../cpp-tftp-server-debug-build"

if (( $# !=1 )); then
    echo "usage: bash fuzzing num_simulated_connections"
    exit 1
fi

num_simulated_connections=$1

echo "Running fuzzing test with $num_simulated_connections concurrent (fuzzy) connections."
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
touch logs.txt
touch error_logs.txt
error_logs_status=$(md5sum error_logs.txt)
$server_binary --tftp-root ../test_transfers --drop 0.0 --duplicate 0.0 --bitflip 0.001 > logs.txt 2>error_logs.txt &
server_pid=$!

echo "Sending the file to the client over TFTP in each of the test dirs."
echo "..."

for i in $( eval echo {1..$num_simulated_connections..1} )
	do
	    if [[ ! -e /proc/$server_pid ]]; then
	        break
	    fi
		cd test_dir_$i
		#NOTE: requires "busybox" package to be installed.
		busybox tftp localhost 3490 -gr beejs_guide.html > /dev/null 2>&1 &
		pids[${i}]=$!
		cd ..
	done

for pid in ${pids[*]}; do
    wait $pid
done

echo "Finished TFTP transfers. Shutting down server."
echo "..."

if [[ ! -e /proc/$server_pid ]]; then
	server_crashed=1
	wait $server_pid 2>/dev/null
else
	kill -9 $server_pid
	wait $server_pid 2>/dev/null
	server_crashed=0
fi

if [[ $error_logs_status != $(md5sum error_logs.txt) ]]; then
	echo "WARNING: the server sent output to standard error."
	echo "Check error_logs.txt for more details."
	echo "..."
fi

echo "Removing test dirs."
echo "..."

for i in $( eval echo {1..$num_simulated_connections..1} )
	do
		rm -r test_dir_$i
	done	

if [[ $server_crashed == 0 ]]; then
	echo "The server did not crash."
	echo "..."
	echo "fuzzing.sh test passed with $num_simulated_connections concurrent (fuzzy) connection(s)."
	exit 0;
else
	echo "The server crashed!"
	echo "..."
	echo "fuzzing.sh test failed with $num_simulated_connections concurrent (fuzzy) connection(s)."
	exit 1;
fi

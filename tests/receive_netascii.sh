#!/bin/bash
server_binary="../cpp-tftp-server-debug-build";

if (( $# !=0 )); then
    echo "usage: bash receive_netascii"
    exit 1
fi

num_simulated_connections=$1

echo "Running test receive_netascii.sh."
echo "..."

echo "Creating test dir to store the received TFTP file."
echo "..."

mkdir test_dir

echo "Starting TFTP server."
echo "..."

#Track if error logs change so we can tell the user of this script
touch error_logs.txt
error_logs_status=$(md5sum error_logs.txt)
$server_binary --tftp-root . --enable-writing 1 >> logs.txt 2>>error_logs.txt &
server_pid=$!

echo "Receiving the file from the client over TFTP in the test dir."
echo "..."

cd ..
cd test_transfers
#NOTE: requires tftp-hpa package as a dependency, not the ordinary debian tftp package,
#as the default tftp package has a "tftp command line" that makes it difficult to
#integrate into bash scripts. 
tftp localhost 3490 -m netascii -c put beejs_guide.html ./test_dir/beejs_guide.html > /dev/null 2>&1 &
wait $!

cd ..
cd tests

echo "Finished TFTP transfer. Shutting down server."
echo "..."

kill -9 $server_pid
wait $server_pid 2>/dev/null

if [[ $error_logs_status != $(md5sum error_logs.txt) ]]; then
	echo "WARNING: the server sent output to standard error."
	echo "Check error_logs.txt for more details."
	echo "..."
fi

echo "Checking for differences between original file and the transferred file(s)."
echo "..."

cd test_dir
#In case the TFTP transfer fails entirely this ensures the empty file is there to diff with
touch beejs_guide.html
differences=$(diff ../../test_transfers/beejs_guide.html beejs_guide.html)
cd ..

echo "Removing test dir."
echo "..."

rm -r test_dir
	
if [[ -z $differences ]]; then
	echo "No differences detected between the transferred file and the original file."
	echo "..."
	echo "Test receive_netascii.sh passed."
else
	echo $differences
	echo "..."
	echo "Differences were detected between the transferred file and the original file."
	echo "This could either have been a bug in the way the server read the files,"
	echo "or it could have been due to connection timeouts."
	echo "..."
	echo "Test receive_netascii.sh failed."
fi

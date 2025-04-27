<h1>Welcome to The Gauntlet</h1>

<p>The scripts "send_concurrent.sh" and "fuzzing.sh"require the package "busybox" to be installed. All of the other scripts require "tftp-hpa" to be installed (not the ordinary "tftp" Debian package). This odd situation is due to the fact that the ordinary "tftp" Debian package puts the user into a "TFTP command line" that makes it difficult to embed into bash scripts, and both tftp and tftp-hpa cannot distinguish different concurrent connections between a client and server on the same computer. busybox can sort of do that, but only when the server is sending files to the client (not when the server is receiving files from the client). Hence, why there isn't a "receive_concurrent.sh" test.</p>

<h3>send.sh: sends a file to the client and diffs it with the original file.</h3>
<h3>send_netascii.sh: same as send.sh but with netascii enabled.</h3>
<h3>send_patchy.sh: same as send.sh but simulates a patchy network connection.</h3>
<h3>send_concurrent.sh: same as send.sh but with multiple connections at once.</h3>
<h3>fuzzing.sh: same as send_concurrent.sh but fuzzes the server to see if it can avoid crashing.</h3>
<h3>receive.sh: receives a file from the client and diffs it with the original file.</h3>
<h3>receive_netascii.sh: same as receive.sh but with netascii enabled.</h3>
<h3>receive_patchy.sh: same as receive.sh but simulates a patchy network connection.</h3>
<h3>clean_logs.sh: removes logs.txt and error_logs.txt.</h3>

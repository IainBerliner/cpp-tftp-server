This repository (cpp-tftp-server) is intended to be an extremely simple, lightweight,
and portable pure c++ repository for building a TFTP server that can be used either as
a command line utility, or as a static lib that can be embedded inside other programs.

The tests directory contains executable Bash scripts for testing the
(testing version) of the binary. These require "busybox" to be installed.
These include:

server_stress_test.sh: tests the server with a given number of simulated
connections. The test succeeds if all connections properly transfer the test
file to a busybox tftp client with no modifications.

server_stress_test_patchy_connection.sh: tests the server with a given number
of simulated (unreliable) connections. This is simulated through macro-dependent
code that adds extra command line options to the testing build, which act as a 
buffer between the code that receives packets and the code that replies to them,
simulating a small packet drop as well as packet duplication percentage.
The test succeeds if all simulated connections properly transfer the test file
to a busybox tftp client with no modifications.

server_stress_test_fuzzing.sh: tests the server with a given number of
simulated connections, where packets experience random bit flips, specified
through macro-dependent code that add an extra command line option to the
testing build, which acts as a buffer between the code that receives packets and
the code that replies to them, simulating a small percentage of random bit flips.
Unlike the other tests, this one succeeds if the simulated connections don't
crash the server.

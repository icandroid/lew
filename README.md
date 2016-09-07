simple wrapper over libevent2
=======================================
the project targets to simplify libevent2 based networking.

to build the project:

1.  change the current directory to the project root director.
2.  mkdir build
3.  cd build
4.  cmake ../
5.  make

to install it, just run `make install`.

to test it, just run `./test_lew`.
to utilize the project, simply include the header files under `include`
directory, and links with library `liblew.a`.

to test C10K, please make sure that the limits of open files has been set
correctly, then run `./c10kserver -l <your host> -p <port>` to be ready to
accept connections, and run `./c10kclient -h <your host> -p <port> -c <count>`
to make connections.
on OSX, the test should be run with root, only root can get enough limits
of open files:
    `launchctl limit maxfiles 1000000 1000000`
    `ulimit -n 20000`

for usage, please check the test cases in file `test/test_tcp_http.cc`.


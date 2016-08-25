simple wrapper over libevent2
=======================================
the project targets to simplify libevent2 based networking.

to build the project:

1.  change the current directory to the project root director.
2.  mkdir build
3.  cd build
4.  cmake ../
5.  make

to test it, just run `./test_lew`.
to utilize the project, simply include the header files under `include` directory,
and links with library `liblew.a`.

for usage, please check the test cases in file `test/test_tcp_http.cc`.


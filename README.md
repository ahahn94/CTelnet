# CTelnet

This is a simple telnet server written in C.  
The server (`server.h, server.c`) takes a pointer to a function so you can pass your own program for the clients to run.  
This project also contains an example for such a program. Have a look at `payload.h` & `payload.c` to get an idea on how to realise your own program.


## Build
To build the program, `cd` into `build` and run `cmake ..` and `make`.  
After that, you can start the program with `./telnet`.
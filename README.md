# Client-Server Socket in C
## Files
`server.c` - C source code for the server

`client.c` - C source code for the client

`tands.c` - C source code for the `Sleep()` and `Trans()` functions

`tands.h` - header file for the `Sleep()` and `Trans()` functions

`server.man` - manual page source code for the server

`client.man` - manual page source code for the client


## Assumptions
The server is not multi-threaded, it serves all clients in a first come first served manner.
Clients will be waiting in a queue while the previous client is being served.

## Approach
The server is implemented as a loop that checks for timeout condition (30 seconds) in each iteration.

The first message from each client is reserved for machine name and pid information.

The client first send its machine name and pid to the server, and loops through every command
from stdin and then send / execute them. If a command is sent, the client waits for server reply.

## Instruction

### Compile and generate files
#### Compile (or Re-compile) everything (clean build, including manual pages)
make
#### Compile server and client only, non-optimized
`make build_all`
#### Compile server and client only, optimized
`make build_all_optimized`
#### Generate manual pages (in pdf)
`make man`
#### Clean
`# clean`
#### Make zip file
`make zip`

### Run
#### Run server
`./server port`
#### Run client
`./client port ip-address`

The client reads in from stdin by default, so if the input is from file, use:

`./client port ip-address < inputfile`

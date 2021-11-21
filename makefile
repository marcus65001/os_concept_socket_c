CC = gcc
CFLAGS = -Wall -g -pthread
CFLAGSOPT = -O -pthread

srv_obj = server.o tands.o
cli_obj = client.o tands.o
default: build_all
build_all: server client
build_all_optimized: server_opt client_opt
server: $(srv_obj)
	$(CC) $(CFLAGS) -o server $(srv_obj)
client: $(cli_obj)
	$(CC) $(CFLAGS) -o client $(cli_obj)
server_opt: $(srv_obj)
	$(CC) $(CFLAGSOPT) -o server $(srv_obj)
client_opt: $(cli_obj)
	$(CC) $(CFLAGSOPT) -o client $(cli_obj)
test: testgen default
zip:
	zip a3.zip client.c server.c tands.c tands.h makefile
clean:
	rm -f *.o client server a3.zip *.log
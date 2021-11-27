CC = gcc
CFLAGS = -O
CFLAGS_DEBUG = -Wall -g

srv_obj = server.o tands.o
cli_obj = client.o tands.o
default: clean build_all_optimized man
build_all: server client
build_all_optimized: server_opt client_opt
server: $(srv_obj)
	$(CC) $(CFLAGS_DEBUG) -o server $(srv_obj)
client: $(cli_obj)
	$(CC) $(CFLAGS_DEBUG) -o client $(cli_obj)
server_opt: $(srv_obj)
	$(CC) $(CFLAGS) -o server $(srv_obj)
client_opt: $(cli_obj)
	$(CC) $(CFLAGS) -o client $(cli_obj)
man:
	groff -Tpdf -man server.man > server.pdf
	groff -Tpdf -man client.man > client.pdf
test: testgen default
zip:
	zip a3.zip client.c server.c tands.c tands.h makefile
clean:
	rm -f *.o client server a3.zip *.log *.pdf
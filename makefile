CC = gcc
CFLAGS = -Wall -g -pthread
CFLAGSOPT = -O -pthread

objects = prodcon.o tands.o
default: all
	$(CC) $(CFLAGS) -o prodcon $(objects)
build_optimized: all
	$(CC) $(CFLAGSOPT) -o prodcon $(objects)
all: $(objects)
test: testgen default
	for run in {1..300}; do ./testgen 10; ./prodcon 3 < t.in; done
	for run in {1..150}; do ./testgen 50; ./prodcon 5 < t.in; done
	for run in {1..80}; do ./testgen 100; ./prodcon 20 < t.in; done
	for run in {1..10}; do ./testgen 200; ./prodcon 25 < t.in; done
	./testgen 255
	valgrind ./prodcon 30 < t.in
zip:
	zip a2.zip prodcon.c tands.c tands.h testgen.c makefile
clean:
	rm -f *.o prodcon testgen a2.zip *.log
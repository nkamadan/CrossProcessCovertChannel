CC=gcc
# CFLAGS=-std=gnu99 #Uncomment if using GCC versions older than 5 (where it may otherwise throw some errors)

all: histogram sender receiver 

fr_util.o: fr_util.c fr_util.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c fr_util.h tx_array.h
	$(CC) $(CFLAGS)  -c $< 

histogram: histogram.o fr_util.o
	$(CC) $(CFLAGS) $^ -o $@

sender: sender.o fr_util.o 
	$(CC) $(CFLAGS) $^ -o $@

receiver: receiver.o fr_util.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o *~ sender receiver histogram


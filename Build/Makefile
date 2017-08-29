SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c = .o)
CC = gcc
INCLUDES = -I.
LIBS = -lpthread
CCFLAGS = -g -Wall -O0

app.bin : $(OBJS)	
	$(CC) $^ -o $@ $(INCLUDES) $(LIBS)	

%.o : %.c
	$(CC) -c $< $(CCFLAGS)

clean:
	rm *.o
.PHONY: clean



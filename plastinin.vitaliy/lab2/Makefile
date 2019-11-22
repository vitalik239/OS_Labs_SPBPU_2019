CC=g++
CFLAGS=-Wall -Werror -std=c++11 -o

all: pipe mq shm

mq:
	$(CC) host.cpp conn_mq.cpp $(CFLAGS) host_mq
	$(CC) client.cpp conn_mq.cpp $(CFLAGS) client_mq

shm:
	$(CC) host.cpp conn_shm.cpp $(CFLAGS) host_shm
	$(CC) client.cpp conn_shm.cpp $(CFLAGS) client_shm
	
pipe:
	$(CC) host.cpp conn_pipe.cpp $(CFLAGS) host_pipe
	$(CC) client.cpp conn_pipe.cpp $(CFLAGS) client_pipe

clean:
	rm -rf host_* client_* *.o
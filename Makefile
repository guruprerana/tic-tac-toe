all: client server

client: client.o
	cc -Wall -g -o client client.o

client.o: client.c
	cc -c -Wall -g client.c

server: server.o
	cc -Wall -g -o server server.o

server.o: server.c
	cc -c -Wall -g server.c

clean:
	rm client client.o server server.o
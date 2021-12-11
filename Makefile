all:
	gcc server.c functions.c -o server -lpthread
	gcc client.c functions.c -o client -lpthread
clean:
	rm -f client server
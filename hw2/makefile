all: svr.c cli.c
	gcc-7 -o client -pthread cli.c
	gcc-7 -o server -pthread svr.c
clear:
	rm -f client server

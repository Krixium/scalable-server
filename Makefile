CC=gcc
CFLAGS=-Wall -ggdb
NAME=server.out
DEBUGNAME=server.out
LINKS=-lpthread

OBJ_FILES=main.o select_svr.o epoll_svr.o net.o tools.o

default: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(NAME) $(LINKS)

debug: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -ggdb -O0 -o $(DEBUGNAME) $(LINKS)

main.o:
	$(CC) $(CFLAGS) -O -c main.c

select_svr.o:
	$(CC) $(CFLAGS) -O -c select_svr.c

epoll_svr.o:
	$(CC) $(CFLAGS) -O -c epoll_svr.c

net.o:
	$(CC) $(CFLAGS) -O -c net.c

tools.o:
	$(CC) $(CFLAGS) -O -c tools.c

clean:
	rm -f *.o *.txt *.log $(NAME) $(DEBUGNAME)

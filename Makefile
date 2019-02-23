# If DEBUG = 1, then the program will build with debugging symbols.
# If DEBUG = 0, then it will not.
DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -ggdb
else
	CFLAGS += -DNDEBUG
endif

CC=gcc
CFLAGS += -Wall -Werror
NAME=server.out
LINKS=-lpthread

SRC := main.c select_svr.c epoll_svr.c net.c tools.c
OBJ := $(SRC:.c=.o)

.PHONY: default clean

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LINKS)

debug: $(OBJ)
	$(CC) $(CFLAGS) -ggdb -o $@ $^ $(LINKS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm -f *.o *.txt *.log $(NAME) $(DEBUGNAME)

count:
	grep new server.log | wc -l
	grep rcv server.log | wc -l
	grep snd server.log | wc -l

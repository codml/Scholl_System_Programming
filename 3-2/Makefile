CC = gcc
NAME = cli srv

all : $(NAME)

%.o : %.c
	$(CC) -g -o $@ $< -lsocket

clean:
	rm -rf $(NAME)
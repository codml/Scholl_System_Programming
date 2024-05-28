CC = gcc
NAME = cli srv

all : $(NAME)

%.o : %.c
	$(CC) -o $@ $< -lsocket

clean:
	rm -rf $(NAME)
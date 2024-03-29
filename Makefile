CC = gcc
NAME = kw2020202034_opt
SRC = kw2020202034_opt.c

all : $(NAME)

$(NAME) : $(SRC)
	$(CC) -o $(NAME) $(SRC)

clean : 
	rm -rf $(NAME)

.PHONY: all clean
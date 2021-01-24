SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))

all:build

build:$(OBJ)

%.o:%.c
	gcc -std=gnu99 -g -c $< -o ../obj/$@

clean:
	cd ../obj && rm -rf $(OBJ)
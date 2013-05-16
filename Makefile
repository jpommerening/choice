example: choice.c choice.h example.c
	$(CC) -Wall -Os -o example choice.c example.c

clean:
	rm -f example

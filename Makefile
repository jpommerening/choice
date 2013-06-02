example: choice.c choice.h example.c
	$(CC) $(CFLAGS) -Wall -o example choice.c example.c

clean:
	rm -f example

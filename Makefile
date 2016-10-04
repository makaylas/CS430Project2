all: raycast.c
	gcc -o raycast raycast.c

clean:
	rm raycast

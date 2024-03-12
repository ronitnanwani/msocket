all:
	gcc -c msocket.c
	ar rcs libmsocket.a msocket.o
	gcc -o initmsocket initmsocket.c -pthread -L. -lmsocket
	gcc -o user1 user1.c -L. -lmsocket
	gcc -o user2 user2.c -L. -lmsocket
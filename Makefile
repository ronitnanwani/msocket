all:
	gcc -c msocket.c
	ar rcs libmsocket.a msocket.o
	gcc -o initmsocket initmsocket.c -pthread -L. -lmsocket
	gcc -o user1 user1.c -L. -lmsocket
	gcc -o user2 user2.c -L. -lmsocket

clean:
	rm initmsocket libmsocket.a msocket.o user1 user2 6000r.txt 6002r.txt 6004r.txt 6006r.txt 6008r.txt 6010r.txt 6012r.txt 6014r.txt 6016r.txt 6018r.txt 6020r.txt 6022r.txt
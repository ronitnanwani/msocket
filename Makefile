all:
	gcc -c msocket.c
	ar rcs libmsocket.a msocket.o
	gcc -o initmsocket initmsocket.c -pthread -L. -lmsocket
	gcc -o user1 user1.c -L. -lmsocket
	gcc -o user2 user2.c -L. -lmsocket
	gcc -o tempuser1 tempuser1.c -L. -lmsocket
	gcc -o tempuser2 tempuser2.c -L. -lmsocket

clean:
	rm initmsocket libmsocket.a msocket.o user1 user2 tempuser1 tempuser2 6000r.txt 6001r.txt 6002r.txt 6003r.txt 6004r.txt 6005r.txt 6006r.txt 6007r.txt 6008r.txt 6009r.txt 6010r.txt 6011r.txt 6012r.txt 6013r.txt 6014r.txt 6015r.txt 6016r.txt 6017r.txt 6018r.txt 6019r.txt 6020r.txt 6021r.txt 6022r.txt 6023r.txt
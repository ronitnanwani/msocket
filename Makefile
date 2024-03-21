all:
	gcc -c msocket.c
	ar rcs libmsocket.a msocket.o
	gcc -o initmsocket initmsocket.c -pthread -L. -lmsocket
	gcc -o user1 user1.c -L. -lmsocket
	gcc -o user2 user2.c -L. -lmsocket
	gcc -o generatefiles generatefiles.c

clean:
	rm initmsocket libmsocket.a msocket.o user1 user2 generatefiles 6000.txt 6001.txt 6002.txt 6003.txt 6004.txt 6005.txt 6006.txt 6007.txt 6008.txt 6009.txt 6010.txt 6011.txt 6012.txt 6013.txt 6014.txt 6015.txt 6016.txt 6017.txt 6018.txt 6019.txt 6020.txt 6021.txt 6022.txt 6023.txt 6000r.txt 6001r.txt 6002r.txt 6003r.txt 6004r.txt 6005r.txt 6006r.txt 6007r.txt 6008r.txt 6009r.txt 6010r.txt 6011r.txt 6012r.txt 6013r.txt 6014r.txt 6015r.txt 6016r.txt 6017r.txt 6018r.txt 6019r.txt 6020r.txt 6021r.txt 6022r.txt 6023r.txt
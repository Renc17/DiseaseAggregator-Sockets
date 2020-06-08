CC = gcc
CFLAGS = -g
Object = master.o
Object1 = Worker.o HashTable.o patientRecord.o RedBlackTree.o List.o
Object2 = whoClient.o
Object3 = whoServer.o

all : clean Worker master
	$(CC) $(CFLAGS) master.c -o master
	$(CC) $(CFLAGS) whoClient.c -o whoClient -lpthread
	$(CC) $(CFLAGS) whoServer.c -o whoServer
	./create_infiles.sh diseaseFile.txt countriesFile.txt input_dir 5 6

Worker:	$(Object1)
	$(CC) $(CFLAGS) Worker.c HashTable.c patientRecord.c RedBlackTree.c List.c -o Worker
hashTable.o : hashTable.h
patientRecord.o : patientRecord.h
RedBlackTree.o : RedBlackTree.h
List.o : List.h

clean:
	rm -f Worker master whoClient whoServer
	rm -f Worker.o HashTable.o patientRecord.o RedBlackTree.o List.o master.o
	rm -f -r input_dir
all: OSS child

OSS: OSS.o scheduler1.o help.o
	gcc OSS.o scheduler1.o help.o -o OSS
	
OSS.o: OSS.c scheduler.h help.h
	gcc -c OSS.c
	
scheduler1.o: scheduler1.c scheduler.h
	gcc -c scheduler1.c

help.o: help.c help.h
	gcc -c help.c

child: child.o
	gcc child.c -o child
	
child.o: child.c
	gcc child.c
	
clean:
	rm *.o

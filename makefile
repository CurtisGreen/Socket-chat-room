default: crc crsd

crc.o: crc.c interface.h
	gcc -c crc.c -o crc.o

crc: crc.o
	gcc crc.o -o crc

crsd.o: crsd.c interface.h
	gcc -c crsd.c -o crsd.o

crsd: crsd.o
	gcc crsd.o -o crsd

clean:
	-rm -f crc.o
	-rm -f crc
	-rm -f crsd.o
	-rm -f crsd
default: crc crsd

crc.o: crc.c interface.h
	g++ -c -pthread crc.c -o crc.o

crc: crc.o
	g++ -pthread crc.o -o crc

crsd.o: crsd.c interface.h
	g++ -pthread -c crsd.c -o crsd.o

crsd: crsd.o
	g++ -pthread crsd.o -o crsd

clean:
	-rm -f crc.o
	-rm -f crc
	-rm -f crsd.o
	-rm -f crsd
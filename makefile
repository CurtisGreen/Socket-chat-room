HEADERS = interface.h

default: crc

crc.o: crc.c $(HEADERS)
    gcc -c crc.c -o crc.o

crc: crc.o
    gcc crc.o -o crc

clean:
    -rm -f crc.o
    -rm -f crc
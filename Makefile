instantlz4.so : 
	g++ -fPIC instantlz4.cpp lz4.c -shared -I/usr/include/python2.6 -I/usr/lib/python2.6/config -L /usr/lib64/ -o instantlz4.so

instantlz4 : instantlz4.o lz4.o
	g++ instantlz4.o lz4.o -I/usr/include/python2.6 -I/usr/lib/python2.6/config -L /usr/lib64/ -o instantlz4

instantlz4.o : lz4.h
	g++ -c instantlz4.cpp

lz4.o : lz4.h
	g++ -c lz4.c

clean:
	rm -rf *.o instantlz4 *.so

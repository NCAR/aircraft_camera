
CSOURCE = main.c getIMG.c parseFile.c writejpeg.c writepng.c
CPPSOURCE = xmp.cpp 
CFLAGS = -Wall
LIBS = -ldc1394 -lraw1394 -ljpeg -lpng -lpq -lexiv2

all: main 

main: xmp $(CSOURCE) 
	gcc -c main.c $(CFLAGS) 
	gcc -c getIMG.c $(CFLAGS)
	gcc -c parseFile.c $(CFLAGS)
	gcc -c writejpeg.c $(CFLAGS) 
	gcc -c writepng.c $(CFLAGS) 
	g++ -o capture main.o getIMG.o parseFile.o writejpeg.o writepng.o xmp.o $(LIBS) 

xmp: xmp.cpp
	g++ -c xmp.cpp 

clean:
	rm -f *.o capture


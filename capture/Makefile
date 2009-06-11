OBJECTS = main.o getIMG.o parseFile.o writejpeg.o writepng.o xmp.o
FLAGS = -Wall
LIBS = -ldc1394 -lraw1394 -ljpeg -lpng -lpq -lexiv2

all: capture

capture: $(OBJECTS) 
	g++ -o capture $(OBJECTS) $(LIBS) $(FLAGS)

main.o: getIMG.h
getIMG.o: getIMG.h writejpeg.h writepng.h
parseFile.o: parseFile.h
writejpeg.o: writejpeg.h
writepng.o: writepng.h
xmp.o: getIMG.h

clean:
	rm -f $(OBJECTS) 


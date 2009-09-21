OBJECTS = main.o getIMG.o parseFile.o writejpeg.o writepng.o pgFuncs.o multicast.o xmp.o
FLAGS = -Wall
DCLIB = -ldc1394
LIBS = $(DCLIB) -ljpeg -lpng -lpq -lexiv2

all: capture capture_reset_bus

capture: $(OBJECTS) 
	$(CXX) -o capture $(OBJECTS) $(LIBS) $(FLAGS)

main.o: getIMG.h
getIMG.o: getIMG.h writejpeg.h writepng.h
parseFile.o: parseFile.h
writejpeg.o: writejpeg.h
writepng.o: writepng.h
xmp.o: getIMG.h
pgFuncs.o: pgFuncs.h
multicast.o: multicast.h

capture_reset_bus:
	$(CC) reset_bus.c -o capture_reset_bus $(DCLIB)

clean:
	rm -f $(OBJECTS) capture capture_reset_bus


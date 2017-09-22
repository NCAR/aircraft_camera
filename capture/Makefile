OBJECTS = main.o getIMG.o parseFile.o writejpeg.o writepng.o pgFuncs.o multicast.o xmp.o
CFLAGS = -Wall -Wextra -g  -pg #-DDEBUG
CXXFLAGS = -Wall -Wextra -g -pg
DCLIB = -ldc1394
LIBS =$(DCLIB) -lpthread -ljpeg -lpng -lpq -lexiv2

all: capture capture_reset_bus

capture: $(OBJECTS) 
	$(CXX) -O2 -g -o capture $(OBJECTS) $(LIBS) $(FLAGS)

main.o: getIMG.h
getIMG.o: getIMG.h writejpeg.h writepng.h
parseFile.o: parseFile.h
writejpeg.o: writejpeg.h
writepng.o: writepng.h
xmp.o: getIMG.h
pgFuncs.o: pgFuncs.h
multicast.o: multicast.h

capture_reset_bus: reset_bus.c
	$(CC) reset_bus.c -o capture_reset_bus $(DCLIB)
install:
	cp capture.sh /etc/init.d
	cp capture /usr/bin
	cp capture_monitor.sh /usr/sbin
uninstall:
	rm -f /etc/init.d/capture.sh
	rm -f /usr/bin/capture
	rm -f /usr/sbin/capture_monitor.sh
clean:
	rm -f $(OBJECTS) capture capture_reset_bus

#
# Link
#

CFLAGS = -Wall -g 

all: bin/audioserver bin/audioguest

bin/lecteur: obj/lecteur.o obj/audio.o
	gcc $(CFLAGS) -o bin/lecteur obj/lecteur.o obj/audio.o

bin/audioserver: obj/audioserver.o obj/audio.o obj/socketlib.o
	gcc $(CFLAGS) -o bin/audioserver obj/audioserver.o obj/audio.o obj/socketlib.o

bin/audioguest: obj/audioguest.o obj/audio.o obj/socketlib.o
	gcc $(CFLAGS) -o bin/audioguest obj/audioguest.o obj/audio.o obj/socketlib.o -lncurses -lm -L/private/student/7/07/11005507/lib/lib -lfftw3
#
# objets of tp lists
#

obj/audio.o: src/audio.c
	gcc $(CFLAGS) -I./include -c src/audio.c -o obj/audio.o

obj/lecteur.o: src/lecteur.c
	gcc $(CFLAGS) -I./include -c src/lecteur.c -o obj/lecteur.o

obj/audioserver.o: src/audioserver.c include/audioserver.h
	gcc $(CFLAGS) -I./include -c src/audioserver.c -o obj/audioserver.o

obj/audioguest.o: src/audioguest.c include/audioguest.h
	gcc $(CFLAGS) -I./include -I/private/student/7/07/11005507/lib/include -c src/audioguest.c -o obj/audioguest.o -lncurses -lm -L/private/student/7/07/11005507/lib/lib -lfftw3


obj/socketlib.o: src/socketlib.c include/socketlib.h
	gcc $(CFLAGS) -I./include -c src/socketlib.c -o obj/socketlib.o

#
# remove files
#

clean :
	rm obj/*.o bin/*



#
# Link
#

CFLAGS = -Wall -g 

all: bin/audioserver bin/audioguest

bin/lecteur: obj/lecteur.o obj/audio.o
	gcc $(CFLAGS) -o bin/lecteur obj/lecteur.o obj/audio.o

bin/audioserver: obj/audioserver.o obj/audio.o obj/socketlib.o
	gcc $(CFLAGS) -o bin/audioserver obj/audioserver.o obj/audio.o obj/socketlib.o

bin/audioguest: obj/audioguest.o obj/audio.o obj/socketlib.o obj/ui.o obj/volume.o obj/visualiser.o obj/equaliser.o
	gcc $(CFLAGS) -o bin/audioguest obj/audioguest.o obj/audio.o obj/socketlib.o obj/ui.o obj/volume.o obj/visualiser.o obj/equaliser.o obj/smplutils.o -lncurses -lfftw3 -lm
#
# objets of tp lists
#

obj/audio.o: src/audio.c
	gcc $(CFLAGS) -I./include -c src/audio.c -o obj/audio.o

obj/lecteur.o: src/lecteur.c
	gcc $(CFLAGS) -I./include -c src/lecteur.c -o obj/lecteur.o

obj/audioserver.o: src/audioserver.c include/audioserver.h include/guestutils.h
	gcc $(CFLAGS) -I./include -c src/audioserver.c -o obj/audioserver.o

obj/audioguest.o: src/audioguest.c include/audioguest.h include/guestutils.h
	gcc $(CFLAGS) -I./include -c src/audioguest.c -o obj/audioguest.o -lncurses -lfftw3 -lm

obj/socketlib.o: src/socketlib.c include/socketlib.h include/guestutils.h
	gcc $(CFLAGS) -I./include -c src/socketlib.c -o obj/socketlib.o

obj/volume.o: src/volume.c include/volume.h include/guestutils.h
	gcc $(CFLAGS) -I./include -c src/volume.c -o obj/volume.o

obj/ui.o: src/ui.c include/ui.h include/guestutils.h
	gcc $(CFLAGS) -I./include -c src/ui.c -o obj/ui.o

obj/visualiser.o: src/visualiser.c include/visualiser.h obj/smplutils.o include/guestutils.h
	gcc $(CFLAGS) -I./include -c src/visualiser.c -o obj/visualiser.o obj/smplutils.o

obj/equaliser.o: src/equaliser.c include/equaliser.h obj/smplutils.o include/guestutils.h
	gcc $(CFLAGS) -I./include -c src/equaliser.c -o obj/equaliser.o obj/smplutils.o

obj/smplutils.o: src/smplutils.c include/smplutils.h include/guestutils.h
	gcc $(CFLAGS) -I./include -c src/smplutils.c -o obj/smplutils.o

#
# remove files
#

clean :
	rm obj/*.o bin/*



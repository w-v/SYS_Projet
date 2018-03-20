#
# Link
#

CFLAGS = -Wall -g

all: bin/audioserver bin/audioguest

bin/lecteur: obj/lecteur.o obj/audio.o
	gcc $(CFLAGS) -o bin/lecteur obj/lecteur.o obj/audio.o

bin/audioserver: obj/audioserver.o
	gcc $(CFLAGS) -o bin/audioserver obj/audioserver.o obj/audio.o

bin/audioguest: obj/audioguest.o
	gcc $(CFLAGS) -o bin/audioguest obj/audioguest.o obj/audio.o
#
# objets of tp lists
#

obj/audio.o: src/audio.c
	gcc $(CFLAGS) -I./include -c src/audio.c -o obj/audio.o

obj/lecteur.o: src/lecteur.c
	gcc $(CFLAGS) -I./include -c src/lecteur.c -o obj/lecteur.o

obj/audioserver.o: src/audioserver.c
	gcc $(CFLAGS) -I./include -c src/audioserver.c -o obj/audioserver.o

obj/audioguest.o: src/audioguest.c
	gcc $(CFLAGS) -I./include -c src/audioguest.c -o obj/audioguest.o

#
# remove files
#

clean :
	rm obj/*.o bin/*



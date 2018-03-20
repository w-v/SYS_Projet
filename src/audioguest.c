#include <sys/types.h>
#include <sys/socket.h>
#include <audio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

int main(){
	int fd;

	int wri;

	int answ = 0;
	struct sockaddr_in to, from;
	char msg[128] = "test.wav";
	char packet[1024];
	socklen_t flen;
	socklen_t len;

	flen = sizeof(struct sockaddr_in);

	fd = socket(AF_INET,SOCK_DGRAM,0);
	if (fd < 0) {
		perror("The socket sucks.");
	}

	to.sin_family = AF_INET;
	to.sin_port = htons(47777);
	to.sin_addr.s_addr = inet_addr("127.0.0.1");

	printf("%s \n", msg);

	len = sendto(fd, msg, sizeof(msg), 0, (struct sockaddr*) &to, flen);
	if (len < 0){
		perror("The sendto doesn't make a good job");
	}
	else{ 
		int info[3];
		len = recvfrom(fd, info, sizeof(info), 0, (struct sockaddr*) &from, &flen); 
		if (len < 0){
			perror("We don't swim in your toilets so don't pee in our pool.");
		} 
		if(info[0] == -1){
			perror("file wasnt found hey!");
		}
		int ok = 1;
		len = sendto(fd, &ok, sizeof(int), 0, (struct sockaddr*) &from, flen);
		if(len < 0){
			perror("Poor guys, you're in trouble");
		}       

		int fdw = aud_writeinit(info[0],info[1],info[2]);
		if(fdw < 0){
			perror("holy hell");
		}       

		do{
			len = recvfrom(fd, packet, sizeof(packet), 0, (struct sockaddr*) &from, &flen);	
			if (len < 0){
				perror("The recvfrom messed up.");
			}


			printf("%s\n",packet );
			wri = write(fdw, packet , 1024);
			if (wri < 0) {
				perror("Too many perror in this script");
				exit(1);
			}

			len = sendto(fd, &ok, sizeof(ok), 0, (struct sockaddr*) &to, flen);
			if (len < 0){
				perror("The sendto doesn't make a good job");
			}

			usleep(1000);
		}while(1);
	}

	return 0;
}

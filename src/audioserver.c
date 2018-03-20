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
#include <dirent.h> 
#include <string.h>

//parser
//sender

int main(){
	int fd, err, rea;
	int i = 0;
	struct sockaddr_in addr, from;
	char msg[128];
	char req_audio[128];
	char * packet = (char*) malloc(1024);
	socklen_t flen;
	socklen_t len;
	//TODO : chemin absolu
	char prefixe[134] = "audio/";

	flen = sizeof(struct sockaddr_in);

	fd = socket(AF_INET,SOCK_DGRAM,0);
	if (fd < 0) {
		perror("The socket sucks.");
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(47777);
	addr.sin_addr.s_addr = htons(INADDR_ANY);

	err = bind(fd, (struct sockaddr *) &addr, flen);
	if (err < 0){
		perror("The bind fucked up.");
	}

	len = recvfrom(fd, msg, sizeof(msg), 0, (struct sockaddr*) &from, &flen);
	if (len < 0){
		perror("The recvfrom messed up.");
	}
	strcat(prefixe,msg);



	if(access(prefixe, F_OK) == -1){
		*packet = -1;
		len = sendto(fd, packet, sizeof(packet), 0, (struct sockaddr*) &from, flen);
		if(len < 0){
			perror("not found is not arrived");
		}
	}
	else{
		char * filename = prefixe;
		int info[3];

		int fdr = aud_readinit( filename,&(info[0]),&(info[1]),&(info[2]));

		printf("%d %d %d",info[0], info[1], info[2]);

		//TODO: un ptit timeout
		len = sendto(fd, info, sizeof(info), 0, (struct sockaddr*) &from, flen);
		if(len < 0){
			perror("Poor guys, you're in trouble");
		}
		len = recvfrom(fd, msg, sizeof(msg), 0, (struct sockaddr*) &from, &flen);
		if (len < 0){
			perror("We don't swim in your toilets so don't pee in our pool.");
		}

		do{
			rea = read(fdr, packet, 1024);

			printf("%s\n",packet );
			if (rea < 0) {
				perror("Houston, we have a problem");
				exit(1);
			}
			len = sendto(fd, packet, sizeof(packet), 0,(struct sockaddr*) &from, flen);
			if(len < 0){
				perror("sendto didn't enjoy life");
			}

			len = recvfrom(fd, msg, sizeof(msg), 0, (struct sockaddr*) &from, &flen);
			if (len < 0){
				perror("The recvfrom messed up.");
			}

			usleep(2000);
		} while (1);

	}
	return 0;
}

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

  struct sockaddr_in to, from;
  char msg[128] = "test.wav";
  char packet[1024];
  socklen_t flen;
  socklen_t len;

  flen = sizeof(struct sockaddr_in);

  fd = socket(AF_INET,SOCK_DGRAM,0);
  if (fd < 0) {
    perror("Could not create socket to server");
  }

  to.sin_family = AF_INET;
  to.sin_port = htons(47777);
  to.sin_addr.s_addr = inet_addr("127.0.0.1");

  len = sendto(fd, msg, sizeof(msg), 0, (struct sockaddr*) &to, flen);
  if (len < 0){
    perror("Could not send filename");
  }
  else{ 
    int info[3];
    len = recvfrom(fd, info, sizeof(info), 0, (struct sockaddr*) &from, &flen); 
    if (len < 0){
      perror("Could not receive wav infos");
    } 
    if(info[0] == -1){
      perror("Server says the file was not found");
      exit(1); 
    }
    int ok = 1;
    len = sendto(fd, &ok, sizeof(int), 0, (struct sockaddr*) &from, flen);
    if(len < 0){
      perror("");
    }       

    int fdw = aud_writeinit(info[0],info[1],info[2]);
    if(fdw < 0){
      perror("Could not get speaker's file descriptor");
    }       

    do{
      len = recvfrom(fd, packet, sizeof(packet), 0, (struct sockaddr*) &from, &flen);	
      if (len < 0){
        perror("Could not receive wav packet");
      }


      wri = write(fdw, packet , 1024);
      if (wri < 0) {
        perror("Could not write to speaker");
        exit(1);
      }

      len = sendto(fd, &ok, sizeof(ok), 0, (struct sockaddr*) &to, flen);
      if (len < 0){
        perror("Could not send ACK");
      }

      usleep(1000);
    }while(1);
  }

  return 0;
}

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

#include <socketlib.h>

int socket_server_init( struct dest_infos* client );
int treat_req(int* fds, struct dest_infos* client, struct request* req_guest);
int recv_req();


#include <socketlib.h>

#define MAX_GUESTS 100

int socket_server_init( struct dest_infos* client );
int treat_req(int* fds, struct dest_infos* client, struct request* req_guest);
int recv_req();
void close_fd(int sig);

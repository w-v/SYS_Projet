#include <audio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char * argv[] ){
  char * filename = argv[1];
  int sample_rate = 0;
  int sample_size = 0;
  int channels = 0;	

  int fdr = aud_readinit( filename, &sample_rate, &sample_size, &channels);


  int fdw = aud_writeinit( sample_rate, sample_size, channels);

  char buf[1024];
  
  int jj = 0;
  int jm = 0;

  do {
    jj = read(fdr, buf, 1024);
    if (jj < 0) {
      perror("JJ not happy");
      exit(1);
    }
    jm = write(fdw, buf, 1024);
    if (jm < 0) {
      perror("JM not happy");
      exit(1);
    }
    usleep(1000);
  } while (jj != 0);

  close(fdr);
  close(fdw);  

  exit(0);
}

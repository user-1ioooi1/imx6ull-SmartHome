#include "sr501.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>

#define sg90Path "/dev/sr501"



void initSr501(struct sr501 *m_sr501){
    m_sr501->fd = open(sg90Path, O_RDWR);
}

int sr501RP(struct sr501 *m_sr501){
    char buf = 0;
    read(m_sr501->fd,&buf,1);
    return (int)buf;
}

void freeSr501(struct sr501 *m_sr501){
    close(m_sr501->fd);
}

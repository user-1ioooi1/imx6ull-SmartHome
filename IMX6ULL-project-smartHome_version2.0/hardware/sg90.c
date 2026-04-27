#include "sg90.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>

#define sg90Path "/dev/sg90"



void initSg90(struct sg90 *m_sg90){
    m_sg90->fd = open(sg90Path, O_RDWR);
    
}

void sg90ChangeAngle(struct sg90 *m_sg90,unsigned char angle){ /* 0-180 */
    write(m_sg90->fd,&angle,sizeof(angle));
}

void freeSg90(struct sg90 *m_sg90){
    close(m_sg90->fd);
}

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>



#include "jdq.h"
#define jdqPath "/dev/jdq"
#define JDQON 1
#define JDQOFF 0


void initJdq(struct jdq *m_jdq){
    m_jdq->fd = open(jdqPath, O_RDWR);
    closeJdq(m_jdq);
}

void jdqStateSwitch(struct jdq *m_jdq,unsigned char status){
    int buf = 0;
    if(status == JDQOFF){
        buf = 0;
    }else{
        buf = 1;
    }
    write(m_jdq->fd, &buf, 1);
}

void openJdq(struct jdq *m_jdq){
    jdqStateSwitch(m_jdq,JDQON);

}

void closeJdq(struct jdq *m_jdq){
    jdqStateSwitch(m_jdq,JDQOFF);
}

void freeJdq(struct jdq *m_jdq){
    close(m_jdq->fd);
}

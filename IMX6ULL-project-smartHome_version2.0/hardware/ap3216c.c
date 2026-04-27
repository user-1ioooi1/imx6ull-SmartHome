#include "ap3216c.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>

#define ap3216cPath "/dev/ap3216c"


void initAp3216c(struct ap3216c * m_ap3216c){
    m_ap3216c->fd = open(ap3216cPath, O_RDWR);
    m_ap3216c->ir = 0;
    m_ap3216c->als = 0;
    m_ap3216c->ps = 0;
}

void ap3216cGetValue(struct ap3216c * m_ap3216c){
    unsigned short data[3] = { 0 };
    read(m_ap3216c->fd,data,sizeof(data));
    m_ap3216c->ir = data[0];
    m_ap3216c->als = data[1];
    m_ap3216c->ps = data[2];
}

unsigned short ap3216cGetIr(struct ap3216c * m_ap3216c){
    return m_ap3216c->ir;
}
unsigned short ap3216cGetAls(struct ap3216c * m_ap3216c){
    return m_ap3216c->als;
}
unsigned short ap3216cGetPs(struct ap3216c * m_ap3216c){
    return m_ap3216c->ps;
}


void freeAp3216c(struct ap3216c * m_ap3216c){
    close(m_ap3216c->fd);
}

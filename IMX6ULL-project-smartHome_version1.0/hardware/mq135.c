#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include "mq135.h"



const char *voltage1_raw = "/sys/bus/iio/devices/iio:device0/in_voltage1_raw";
const char *voltage_scale = "/sys/bus/iio/devices/iio:device0/in_voltage_scale";

void initMq135(struct mq135 *m_mq135){
    m_mq135->raw_fd = open(voltage1_raw, O_RDWR);
    m_mq135->scale_fd =  open(voltage_scale, O_RDWR);
    m_mq135->scale =0;
    m_mq135->raw = 0;
}

void mq135RD(struct mq135 *m_mq135){
    lseek(m_mq135->scale_fd, 0, SEEK_SET);
    read(m_mq135->scale_fd,m_mq135->data,20);
    m_mq135->scale = atof(m_mq135->data);
    lseek(m_mq135->raw_fd, 0, SEEK_SET);
    read(m_mq135->raw_fd,m_mq135->data,20);
    m_mq135->raw = atoi(m_mq135->data);
}


int mq135Rraw(struct mq135 *m_mq135){
    return m_mq135->raw;
    
}

double mq135Rscale(struct mq135 *m_mq135){
    return m_mq135->scale;
}

void freeMq135(struct mq135 *m_mq135){
    close(m_mq135->raw_fd);
    close(m_mq135->scale_fd);
}



#include "dht11.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>

#define dht11Path "/dev/dht11"


void initDht11(struct dht11 *m_dht11){
    m_dht11->fd = open(dht11Path,O_RDWR);
    m_dht11->humidity = 0;
    m_dht11->tempature = 0;
}

short dht11_getHumidity(struct dht11 *m_dht11){
    return m_dht11->humidity;
}
short dht11_gettempature(struct dht11 *m_dht11){
    return m_dht11->tempature;
}

void dht11_getData(struct dht11 *m_dht11){
    char buf[4] = { 0 };
    if(read(m_dht11->fd,buf,4) > 0){
        m_dht11->humidity = buf[0];
        m_dht11->tempature = buf[2];
    }
}

void freeDht11(struct dht11 *m_dht11){
    close(m_dht11->fd);
}

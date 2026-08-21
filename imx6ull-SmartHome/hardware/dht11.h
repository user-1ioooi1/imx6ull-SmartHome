#ifndef DHT11_H
#define DHT11_H

struct dht11 {
    int fd;
    short humidity, tempature;
};


#ifdef __cplusplus
extern "C" {
#endif

void initDht11(struct dht11 *m_dht11);

short dht11_getHumidity(struct dht11 *m_dht11);
short dht11_gettempature(struct dht11 *m_dht11);
void dht11_getData(struct dht11 *m_dht11);
void freeDht11(struct dht11 *m_dht11);

#ifdef __cplusplus
}
#endif


#endif

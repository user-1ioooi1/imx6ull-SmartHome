#ifndef sr501_H
#define sr501_H


struct sr501 {
    int fd;
};

#ifdef __cplusplus
extern "C" {
#endif

void initSr501(struct sr501 *m_sr501);

int sr501RP(struct sr501 *m_sr501);

void freeSr501(struct sr501 *m_sr501);

#ifdef __cplusplus
}
#endif

#endif
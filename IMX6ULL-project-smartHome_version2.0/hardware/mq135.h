#ifndef mq135_H
#define mq135_H


struct mq135 {
    int raw_fd;
    int scale_fd;
    int raw;
    double scale;
    char data[20];
};

#ifdef __cplusplus
extern "C" {
#endif

void initMq135(struct mq135 *m_mq135);

void mq135RD(struct mq135 *m_mq135);

double mq135Rscale(struct mq135 *m_mq135);

int mq135Rraw(struct mq135 *m_mq135);

void freeMq135(struct mq135 *m_mq135);

#ifdef __cplusplus
}
#endif

#endif

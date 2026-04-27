#ifndef SG90_H
#define SG90_H


struct sg90 {
    int fd;
};

#ifdef __cplusplus
extern "C" {
#endif

void initSg90(struct sg90 *m_sg90);

void sg90ChangeAngle(struct sg90 *m_sg90,unsigned char angle);

void freeSg90(struct sg90 *m_sg90);

#ifdef __cplusplus
}
#endif

#endif



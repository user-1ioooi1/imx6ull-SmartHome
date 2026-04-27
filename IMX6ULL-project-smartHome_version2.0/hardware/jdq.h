#ifndef JDQ_H
#define JDQ_H

struct jdq {
    int fd;
};


#ifdef __cplusplus
extern "C" {
#endif

void initJdq(struct jdq *m_jdq);

void jdqStateSwitch(struct jdq *m_jdq,unsigned char status);

void openJdq(struct jdq *m_jdq);

void closeJdq(struct jdq *m_jdq);

void freeJdq(struct jdq *m_jdq);

#ifdef __cplusplus
}
#endif

#endif


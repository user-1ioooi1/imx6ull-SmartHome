#ifndef AP3216C_H
#define AP3216C_H

struct ap3216c {
    int fd;
    unsigned short ir;
    unsigned short als;
    unsigned short ps;
};


#ifdef __cplusplus
extern "C" {
#endif


void initAp3216c(struct ap3216c * m_ap3216c);

void ap3216cGetValue(struct ap3216c * m_ap3216c);

unsigned short ap3216cGetIr(struct ap3216c * m_ap3216c);
unsigned short ap3216cGetAls(struct ap3216c * m_ap3216c);
unsigned short ap3216cGetPs(struct ap3216c * m_ap3216c);

void freeAp3216c(struct ap3216c * m_ap3216c);

#ifdef __cplusplus
}
#endif


#endif

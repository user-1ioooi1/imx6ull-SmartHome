#ifndef Beep_H
#define Beep_H

struct beep {
    int fd;
};


#ifdef __cplusplus
extern "C" {
#endif

void initBeep(struct beep *m_beep);

void beepStateSwitch(struct beep *m_beep,unsigned char status);

void openBeep(struct beep *m_beep);

void closeBeep(struct beep *m_beep);

void freeBeep(struct beep *m_beep);

#ifdef __cplusplus
}
#endif


#endif

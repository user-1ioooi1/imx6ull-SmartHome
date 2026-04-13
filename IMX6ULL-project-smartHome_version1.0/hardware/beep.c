#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include "beep.h"

#define beepPath "/dev/beep"
#define BEEPON 1
#define BEEPOFF 0



void initBeep(struct beep *m_beep){
    m_beep->fd = open(beepPath, O_RDWR);
    closeBeep(m_beep);
}

void beepStateSwitch(struct beep *m_beep,unsigned char status){
    int buf = 0;
    if(status == BEEPOFF){
        buf = 1;
    }else{
        buf = 0;
    }
    write(m_beep->fd, &buf, 1);
}

void openBeep(struct beep *m_beep){
    beepStateSwitch(m_beep,BEEPON);

}

void closeBeep(struct beep *m_beep){
    beepStateSwitch(m_beep,BEEPOFF);
}

void freeBeep(struct beep *m_beep){
    close(m_beep->fd);
}

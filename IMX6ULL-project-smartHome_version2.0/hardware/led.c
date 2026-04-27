#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>



#include "led.h"
#define ledPath "/dev/gpioled"
#define LEDON 1
#define LEDOFF 0



    
void initLed(struct led *m_led){
    m_led->fd = open(ledPath, O_RDWR);
    closeLed(m_led);
}

void ledStateSwitch(struct led *m_led,unsigned char status){
    int buf = 0;
    if(status == LEDOFF){
        buf = 1;
    }else{
        buf = 0;
    }
    write(m_led->fd, &buf, 1);
}

void openLed(struct led *m_led){
    ledStateSwitch(m_led,LEDON);

}

void closeLed(struct led *m_led){
    ledStateSwitch(m_led,LEDOFF);
}

void freeLed(struct led *m_led){
    close(m_led->fd);
}

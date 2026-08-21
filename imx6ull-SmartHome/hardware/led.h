#ifndef LED_H
#define LED_H

struct led {
    int fd;
};


#ifdef __cplusplus
extern "C" {
#endif

void initLed(struct led *m_led);

void ledStateSwitch(struct led *m_led,unsigned char status);

void openLed(struct led *m_led);

void closeLed(struct led *m_led);

void freeLed(struct led *m_led);

#ifdef __cplusplus
}
#endif

#endif

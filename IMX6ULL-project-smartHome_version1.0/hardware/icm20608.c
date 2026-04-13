#include "icm20608.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>

#define icm20608Path "/dev/icm20608"



void initIcm20608(struct icm20608* m_icm20608){
    m_icm20608->fd = open(icm20608Path, O_RDWR);
    m_icm20608->gyro_x_adc  =   0;
    m_icm20608->gyro_y_adc  =   0;
    m_icm20608->gyro_z_adc  =   0;
    m_icm20608->accel_x_adc =   0;
    m_icm20608->accel_y_adc =   0;
    m_icm20608->accel_z_adc =   0;
    m_icm20608->tempature   =   0;
}

void icm20608GetValue(struct icm20608* m_icm20608){
    int data[7] = { 0 };
    read(m_icm20608->fd,data,sizeof(data));
    m_icm20608->gyro_x_adc  =   data[0] / 16.4;
    m_icm20608->gyro_y_adc  =   data[1] / 16.4;
    m_icm20608->gyro_z_adc  =   data[2] / 16.4;
    m_icm20608->accel_x_adc =   (float)data[3] / 2048;
    m_icm20608->accel_y_adc =   (float)data[4] / 2048;
    m_icm20608->accel_z_adc =   (float)data[5] / 2048;
    m_icm20608->tempature   =   (data[6] - 25) / 326.8 + 25;
}

float icm20608Get_Gyro_x_adc(struct icm20608* m_icm20608){
    return m_icm20608->gyro_x_adc;
}
float icm20608Get_Gyro_y_adc(struct icm20608* m_icm20608){
    return m_icm20608->gyro_y_adc;
}
float icm20608Get_Gyro_z_adc(struct icm20608* m_icm20608){
    return m_icm20608->gyro_z_adc;
}

float icm20608Get_accel_x_adc(struct icm20608* m_icm20608){
    return m_icm20608->accel_x_adc;
}
float icm20608Get_accel_y_adc(struct icm20608* m_icm20608){
    return m_icm20608->accel_y_adc;
}
float icm20608Get_accel_z_adc(struct icm20608* m_icm20608){
    return m_icm20608->accel_z_adc;
}

float icm20608Get_tempature(struct icm20608* m_icm20608){
    return m_icm20608->tempature;
}


void freeIcm20608(struct icm20608* m_icm20608){
    close(m_icm20608->fd);
}


#ifndef ICM20608_H
#define ICM20608_H



struct icm20608 {
    int fd;
    float gyro_x_adc,gyro_y_adc,gyro_z_adc,accel_x_adc ,
            accel_y_adc,accel_z_adc , tempature;
};

#ifdef __cplusplus
extern "C" {
#endif

void initIcm20608(struct icm20608* m_icm20608);


void icm20608GetValue(struct icm20608* m_icm20608);

float icm20608Get_Gyro_x_adc(struct icm20608* m_icm20608);


float icm20608Get_Gyro_y_adc(struct icm20608* m_icm20608);


float icm20608Get_Gyro_z_adc(struct icm20608* m_icm20608);


float icm20608Get_accel_x_adc(struct icm20608* m_icm20608);

float icm20608Get_accel_y_adc(struct icm20608* m_icm20608);

float icm20608Get_accel_z_adc(struct icm20608* m_icm20608);

float icm20608Get_tempature(struct icm20608* m_icm20608);

void freeIcm20608(struct icm20608* m_icm20608);

#ifdef __cplusplus
}
#endif


#endif

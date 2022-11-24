/**
 * @file tuya_imu_daq.c
 * @author lifan
 * @brief IMU DAQ (Data Acquisition) module source file
 * @version 1.0.0
 * @date 2021-12-11
 *
 * @copyright Copyright (c) tuya.inc 2021
 *
 */

#include "tuya_imu_daq.h"
#include "tuya_mpu6050.h"
#include "tuya_ble_port.h"
#include "tuya_ble_log.h"
#include "ty_pin.h"

/***********************************************************
************************micro define************************
***********************************************************/
#define DAQ_TIME_MS         5

#define MPU_CT_POW_PIN      GPIO_P07  //控制6050供电
#define MPU_INT_PIN         GPIO_P11  //6050int

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
STATIC DAQ_END_CB sg_daq_end_cb = NULL;
STATIC tuya_ble_timer_t daq_timer;

/***********************************************************
***********************function define**********************
***********************************************************/
/**
 * @brief sensor new data ready callback
 * @param[in] none
 * @return none
 */
STATIC VOID_T __new_data_ready_cb(VOID_T)
{
    if (sg_daq_end_cb != NULL) {
        sg_daq_end_cb();
    }
}

/**
 * @brief IMU DAQ module init  IMU数据采集模块初始化
 * @param[in] daq_end_cb: data acquisition end callback 数据采集回调
 * @return none
 */
VOID_T tuya_imu_daq_init(DAQ_END_CB daq_end_cb)//ok
{
	  //TUYA_APP_LOG_DEBUG("tuya_imu_daq_init0");
    tuya_mpu6050_power_on(MPU_CT_POW_PIN, TRUE);
#if INV_MOTION_DRIVER
    MPU_RET ret = tuya_mpu6050_dmp_init(MPU_GYRO_FS_2000, MPU_ACCEL_FS_16, MPU_INT_PIN, TY_GPIO_IRQ_FALLING, __new_data_ready_cb);
	  TUYA_APP_LOG_DEBUG("tuya_mpu6050_dmp_init");
#else
    MPU_RET ret = tuya_mpu6050_init(MPU_CLK_PLL_XGYRO, MPU_GYRO_FS_2000, MPU_ACCEL_FS_16, 1000/DAQ_TIME_MS, MPU_INT_PIN, TY_GPIO_IRQ_FALLING, __new_data_ready_cb);
	  //TUYA_APP_LOG_DEBUG("tuya_mpu6050_init");
    tuya_ble_timer_create(&daq_timer, 5, TUYA_BLE_TIMER_REPEATED, __new_data_ready_cb);
    tuya_ble_timer_start(daq_timer);
	  
#endif
    if (MPU_OK != ret) {
        TUYA_APP_LOG_ERROR("tuya_mpu6050_init error: %d.", ret);
        return;
        }
    sg_daq_end_cb = daq_end_cb;
		//TUYA_APP_LOG_DEBUG("tuya_imu_daq_init1");
}

/**
 * @brief close IMU DAQ
 * @param[in] none
 * @return none
 */
VOID_T tuya_close_imu_daq(VOID_T)
{
    tuya_mpu6050_set_sleep_mode(TRUE);
    tuya_mpu6050_power_off(MPU_CT_POW_PIN, TRUE);
}

/**
 * @brief get IMU data
 * @param[in] gyro: gyro data
 * @param[in] accel: accel data
 * @param[in] imu_angle: angle data from IMU
 * @return TRUE - success, FALSE - fail
 */
BOOL_T tuya_get_imu_data(FLOAT_T *gyro, FLOAT_T *accel, FLOAT_T *imu_angle)
{
#if INV_MOTION_DRIVER
    //读取MPU内置单元的数据
    if (tuya_mpu6050_read_dmp_spec_unit(MPU_GDT_DPS, gyro, MPU_ADT_MPS2, accel, imu_angle, imu_angle+1, imu_angle+2)) {
        return FALSE;
    }
#else
		//从陀螺仪读取指定数据
    tuya_mpu6050_read_gyro_spec_unit(gyro+1, gyro, gyro+2, MPU_GDT_DPS);
		//读取加速计数据
    tuya_mpu6050_read_accel_spec_unit(accel+1, accel, accel+2, MPU_ADT_MPS2);
    gyro[1] = -gyro[1];
    accel[1] = -accel[1];
#endif
    return TRUE;
}

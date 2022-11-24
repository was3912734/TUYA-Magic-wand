/**
 * @file tuya_key.h
 * @author lifan
 * @brief key driver header file
 * @version 1.0.0
 * @date 2021-09-23
 *
 * @copyright Copyright (c) tuya.inc 2021
 *
 */

#ifndef __TUYA_KEY_H__
#define __TUYA_KEY_H__

#include "tuya_common.h"
#include "tuya_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************micro define************************
***********************************************************/

/***********************************************************
***********************typedef define***********************
***********************************************************/
/*定义按键错误信息代码 */
typedef BYTE_T KEY_RET;
#define KEY_OK                  0x00
#define KEY_ERR_MALLOC_FAILED   0x01
#define KEY_ERR_CB_UNDEFINED    0x02

//定义供用户注册按键信息使用的结构体类型 
/* 按键事件类型 */
typedef BYTE_T KEY_PRESS_TYPE_E;
#define SHORT_PRESS             0x00
#define LONG_PRESS_FOR_TIME1    0x01
#define LONG_PRESS_FOR_TIME2    0x02
/* 按键回调函数类型 */
typedef VOID_T (*KEY_CALLBACK)(KEY_PRESS_TYPE_E type);
/* 按键注册信息类型 */
typedef struct {                /* user define */
    gpio_pin_e port;        /* key port */
    BOOL_T active_low;          /* key detection is active low? */
    UINT_T long_press_time1;    /* key long press time1 set (ms) */
    UINT_T long_press_time2;    /* key long press time2 set (ms) */
    KEY_CALLBACK key_cb;        /* key press callback function */
} KEY_DEF_T;

/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define**********************
***********************************************************/
/**
 * @brief key register
 * @param[in] key_def: user key define
 * @return KEY_RET
 */
//在头文件中定义按键注册接口
KEY_RET tuya_reg_key(_IN KEY_DEF_T* key_def);

/**
 * @brief key reset
 * @param[in] none
 * @return KEY_RET
 */
KEY_RET tuya_key_reset(VOID_T);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TUYA_KEY_H__ */

/**
 * @file tuya_gpio.h
 * @author lifan
 * @brief tuya gpio header file
 * @version 1.0.1
 * @date 2022-01-26
 *
 * @copyright Copyright (c) tuya.inc 2021
 *
 */

#ifndef __TUYA_GPIO_H__
#define __TUYA_GPIO_H__

#include "tuya_common.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************micro define************************
***********************************************************/
typedef BYTE_T GPIO_RET;
#define GPIO_OK                 0x00
#define GPIO_ERR_INVALID_PARM   0x01
#define GPIO_ERR_MALLOC_FAILED  0x02
#define GPIO_ERR_CB_UNDEFINED   0x03

/***********************************************************
***********************typedef define***********************
***********************************************************/
/*
	typedef enum {
    TY_GPIO_0 = 0,
    TY_GPIO_1,
    TY_GPIO_2,
    TY_GPIO_3,
    TY_GPIO_4,
    TY_GPIO_5,
    TY_GPIO_6,
    TY_GPIO_7,
    TY_GPIO_8,
    TY_GPIO_9,
    TY_GPIO_10,
    TY_GPIO_11,
    TY_GPIO_12,
    TY_GPIO_13,
    TY_GPIO_14,
    TY_GPIO_15,
    TY_GPIO_16,
    TY_GPIO_17,
    TY_GPIO_18,
    TY_GPIO_19,
    TY_GPIO_20,
    TY_GPIO_21,
    TY_GPIO_22,
    TY_GPIO_23,
    TY_GPIO_24,
    TY_GPIO_25,
    TY_GPIO_26,
    TY_GPIO_27,
    TY_GPIO_28,
    TY_GPIO_29,
    TY_GPIO_30,
    TY_GPIO_31,
    TY_GPIO_32,
    TY_GPIO_33,
    TY_GPIO_34,
    TY_GPIO_MAX
} TY_GPIO_PORT_E;
*/
/*
typedef enum {
    GPIO_P00   =   0,    P0  =  GPIO_P00,
    GPIO_P01   =   1,    P1  =  GPIO_P01,
    GPIO_P02   =   2,    P2  =  GPIO_P02,
    GPIO_P03   =   3,    P3  =  GPIO_P03,
    GPIO_P07   =   4,    P7  =  GPIO_P07,
    GPIO_P09   =   5,    P9  =  GPIO_P09,
    GPIO_P10   =   6,    P10  =  GPIO_P10,
    GPIO_P11   =   7,    P11  =  GPIO_P11,   Analog_IO_0 = GPIO_P11,
    GPIO_P14   =   8,    P14  =  GPIO_P14,   Analog_IO_1 = GPIO_P14,
    GPIO_P15   =   9,    P15  =  GPIO_P15,   Analog_IO_2 = GPIO_P15,
    GPIO_P16   =   10,   P16  =  GPIO_P16,   Analog_IO_3 = GPIO_P16,XTALI = GPIO_P16,
    GPIO_P17   =   11,   P17  =  GPIO_P17,   Analog_IO_4 = GPIO_P17,XTALO = GPIO_P17,
    GPIO_P18   =   12,   P18  =  GPIO_P18,   Analog_IO_5 = GPIO_P18,
    GPIO_P20   =   13,   P20  =  GPIO_P20,   Analog_IO_6 = GPIO_P20,
    GPIO_P23   =   14,   P23  =  GPIO_P23,   Analog_IO_7 = GPIO_P23,
    GPIO_P24   =   15,   P24  =  GPIO_P24,   Analog_IO_8 = GPIO_P24,
    GPIO_P25   =   16,   P25  =  GPIO_P25,   Analog_IO_9 = GPIO_P25,
    GPIO_P26   =   17,   P26  =  GPIO_P26,
    GPIO_P27   =   18,   P27  =  GPIO_P27,
    GPIO_P31   =   19,   P31  =  GPIO_P31,
    GPIO_P32   =   20,   P32  =  GPIO_P32,
    GPIO_P33   =   21,   P33  =  GPIO_P33,
    GPIO_P34   =   22,   P34  =  GPIO_P34,
    GPIO_NUM   =   23,
		TY_GPIO_MAX =  24,
    GPIO_DUMMY =  0xff,
} gpio_pin_e;
*/
typedef BYTE_T TY_GPIO_MODE_E;
#define TY_GPIO_PULLUP      0x00
#define TY_GPIO_PULLDOWN    0x01
#define TY_GPIO_FLOATING    0x02

typedef BYTE_T TY_GPIO_IRQ_TYPE_E;
#define TY_GPIO_IRQ_NONE    0x00
#define TY_GPIO_IRQ_RISING  0x01
#define TY_GPIO_IRQ_FALLING 0x02

typedef VOID_T (*TY_GPIO_IRQ_CB)();

/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define**********************
***********************************************************/
/**
 * @brief tuya gpio init
 * @param[in] port: gpio number
 * @param[in] in: TRUE - in, FALSE - out
 * @param[in] active_low: TRUE - active_low, FALSE - active high
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_init(_IN CONST gpio_pin_e port, _IN CONST BOOL_T in, _IN CONST BOOL_T active_low);

/**
 * @brief tuya gpio write
 * @param[in] port: gpio number
 * @param[in] level: output level
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_write(_IN CONST gpio_pin_e port, _IN CONST BOOL_T level);

/**
 * @brief tuya gpio read
 * @param[in] port: gpio number
 * @return TRUE - high level, false - low level
 */
BOOL_T tuya_gpio_read(_IN CONST gpio_pin_e port);

/**
 * @brief tuya gpio interrupt init
 * @param[in] port: gpio number
 * @param[in] trig_type: trigger type
 * @param[in] irq_cb: interrupt callback function
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_irq_init(_IN CONST gpio_pin_e port, _IN CONST TY_GPIO_IRQ_TYPE_E trig_type, _IN TY_GPIO_IRQ_CB irq_cb);

/**
 * @brief tuya gpio close
 * @param[in] port: gpio number
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_close(_IN CONST gpio_pin_e port);

/**
 * @brief tuya gpio close all
 * @param[in] none
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_close_all(VOID_T);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TUYA_GPIO_H__ */

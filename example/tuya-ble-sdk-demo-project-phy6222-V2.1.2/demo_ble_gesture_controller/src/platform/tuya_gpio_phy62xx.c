/**
 * @file tuya_gpio_phy62xx.c
 * @author lifan
 * @brief tuya gpio source file for PHY62xx
 * @version 1.0.0
 * @date 2022-01-26
 *
 * @copyright Copyright (c) tuya.inc 2021
 *
 */

#include "tuya_gpio.h"
#include "tuya_ble_mem.h"
#include "gpio.h"

/***********************************************************
************************micro define************************
***********************************************************/

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct ty_gpio_irq_mag_s {
    struct ty_gpio_irq_mag_s *next;
    gpio_pin_e port;
    TY_GPIO_IRQ_CB irq_cb;
} TY_GPIO_IRQ_MAG_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
/*
STATIC gpio_pin_e sg_pf_pin_list[] = {
    GPIO_P00,
    GPIO_P01,
    GPIO_P02,
    GPIO_P03,
    GPIO_DUMMY,
    GPIO_DUMMY,
    GPIO_DUMMY,
    GPIO_P07,
    GPIO_DUMMY,
    GPIO_P09,
    GPIO_P10,
    GPIO_P11,
    GPIO_DUMMY,
    GPIO_DUMMY,
    GPIO_P14,
    GPIO_P15,
    GPIO_P16,
    GPIO_P17,
    GPIO_P18,
    GPIO_DUMMY,
    GPIO_P20,
    GPIO_DUMMY,
    GPIO_DUMMY,
    GPIO_P23,
    GPIO_P24,
    GPIO_P25,
    GPIO_P26,
    GPIO_P27,
    GPIO_DUMMY,
    GPIO_DUMMY,
    GPIO_DUMMY,
    GPIO_P31,
    GPIO_P32,
    GPIO_P33,
    GPIO_P34,
    GPIO_DUMMY
};
*/

STATIC UINT_T sg_pf_pin_list[] = {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23
};

STATIC TY_GPIO_IRQ_MAG_T *sg_irq_mag_list = NULL;

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
GPIO_RET tuya_gpio_init(_IN CONST gpio_pin_e port, _IN CONST BOOL_T in, _IN CONST BOOL_T active_low)
{
    if ((port >= TY_GPIO_MAX) ||
        (-1 == sg_pf_pin_list[port])) {
        return GPIO_ERR_INVALID_PARM;
    }

    if (in) {
        hal_gpio_pin_init(sg_pf_pin_list[port], GPIO_INPUT);
        if (active_low) {
            hal_gpio_pull_set(sg_pf_pin_list[port], GPIO_PULL_UP);
        } else {
            hal_gpio_pull_set(sg_pf_pin_list[port], GPIO_PULL_DOWN);
        }
    } else {
        hal_gpio_pin_init(sg_pf_pin_list[port], GPIO_OUTPUT);
        if (active_low) {
            hal_gpio_write(sg_pf_pin_list[port], TRUE);
        } else {
            hal_gpio_write(sg_pf_pin_list[port], FALSE);
        }
    }

    return GPIO_OK;
}

/**
 * @brief tuya gpio write
 * @param[in] port: gpio number
 * @param[in] level: output level
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_write(_IN CONST gpio_pin_e port, _IN CONST BOOL_T level)
{
    if ((port >= TY_GPIO_MAX) ||
        (-1 == sg_pf_pin_list[port])) {
        return GPIO_ERR_INVALID_PARM;
    }

    hal_gpio_write(sg_pf_pin_list[port], level);

    return GPIO_OK;
}

/**
 * @brief tuya gpio read
 * @param[in] port: gpio number
 * @return TRUE - high level, false - low level
 */
BOOL_T tuya_gpio_read(_IN CONST gpio_pin_e port)
{
    if ((port >= TY_GPIO_MAX) ||
        (-1 == sg_pf_pin_list[port])) {
        return GPIO_ERR_INVALID_PARM;
    }

    return hal_gpio_read(sg_pf_pin_list[port]);
}

/**
 * @brief gpio irq handler
 * @param[in] pin: pin number
 * @param[in] type: gpiote event polarity
 * @return none
 */
/*
VOID_T __gpio_irq_handler(gpio_pin_e pin, gpio_polarity_e type)
{
    TY_GPIO_IRQ_MAG_T *irq_mag_tmp = sg_irq_mag_list;
    while (irq_mag_tmp) {
        if (pin == sg_pf_pin_list[irq_mag_tmp->port]) {
            irq_mag_tmp->irq_cb();
        }
        irq_mag_tmp = irq_mag_tmp->next;
    }
}
*/
VOID_T __gpio_irq_handler(gpio_pin_e pin, gpio_polarity_e type)
{
    TY_GPIO_IRQ_MAG_T *irq_mag_tmp = sg_irq_mag_list;
    while (irq_mag_tmp) {
        if ((pin == sg_pf_pin_list[irq_mag_tmp->port]) &&
            (type == NEGEDGE)) {
            irq_mag_tmp->irq_cb();
        }
        irq_mag_tmp = irq_mag_tmp->next;
    }
}

/**
 * @brief tuya gpio interrupt init
 * @param[in] port: gpio number
 * @param[in] trig_type: trigger type
 * @param[in] irq_cb: interrupt callback function
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_irq_init(_IN CONST gpio_pin_e port, _IN CONST TY_GPIO_IRQ_TYPE_E trig_type, _IN TY_GPIO_IRQ_CB irq_cb)
{
    if ((port >= TY_GPIO_MAX) ||
        (-1 == sg_pf_pin_list[port]) ||
        (trig_type > TY_GPIO_IRQ_FALLING) ||
        (irq_cb == NULL)) {
        return GPIO_ERR_INVALID_PARM;
    }

    TY_GPIO_IRQ_MAG_T *irq_mag_tmp = (TY_GPIO_IRQ_MAG_T *)tuya_ble_malloc(SIZEOF(TY_GPIO_IRQ_MAG_T));
    if (NULL == irq_mag_tmp) {
        return GPIO_ERR_MALLOC_FAILED;
    }

    irq_mag_tmp->port = port;
    irq_mag_tmp->irq_cb = irq_cb;
    if (sg_irq_mag_list) {
        irq_mag_tmp->next = sg_irq_mag_list;
    }
    sg_irq_mag_list = irq_mag_tmp;

    switch (trig_type) {
    case TY_GPIO_IRQ_NONE:
        
        break;
    case TY_GPIO_IRQ_RISING:
        hal_gpio_pull_set(sg_pf_pin_list[port], GPIO_PULL_DOWN);
        hal_gpioin_register(sg_pf_pin_list[port], __gpio_irq_handler, NULL);
        break;
    case TY_GPIO_IRQ_FALLING:
        hal_gpio_pull_set(sg_pf_pin_list[port], GPIO_PULL_UP);
        hal_gpioin_register(sg_pf_pin_list[port], NULL, __gpio_irq_handler);
        break;
    default:
        break;
    }

    return GPIO_OK;
}

/**
 * @brief tuya gpio close
 * @param[in] port: gpio number
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_close(_IN CONST gpio_pin_e port)
{
#if 0
    if ((port >= TY_GPIO_MAX) ||
        (-1 == sg_pf_pin_list[port])) {
        return GPIO_ERR_INVALID_PARM;
    }
#endif
    return GPIO_OK;
}

/**
 * @brief tuya gpio close all
 * @param[in] none
 * @return GPIO_RET
 */
GPIO_RET tuya_gpio_close_all(VOID_T)
{
#if 0
    UINT_T pin;
    for (pin = 0; pin < NUMBER_OF_PINS; pin++) {
    }
#endif
    return GPIO_OK;
}

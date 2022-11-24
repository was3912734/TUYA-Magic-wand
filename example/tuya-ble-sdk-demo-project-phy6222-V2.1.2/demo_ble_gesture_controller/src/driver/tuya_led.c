/**
 * @file tuya_led.c
 * @author lifan
 * @brief led driver source file
 * @version 1.0.0
 * @date 2021-09-23
 *
 * @copyright Copyright (c) tuya.inc 2021
 *
 */

#include "tuya_led.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_port.h"

/***********************************************************
************************micro define************************
***********************************************************/
#define LED_TIMER_VAL_MS    100

/***********************************************************
***********************typedef define***********************
***********************************************************/
/* �ڶ��������������ע���LED���������Ϣ (tuya_led.c) */
typedef struct {
    gpio_pin_e pin;             /* led pin */
    BOOL_T active_low;              /* led light on is active low? */
} LED_DRV_T;

/* ��һ�����������ڹ���LED��˸��Ϣ�Ľṹ������ (tuya_led.c) */
typedef struct {
    LED_FLASH_MODE_E mode;          /* ��˸��ʽ */
    LED_FLASH_TYPE_E type;          /* ��˸���� */
    USHORT_T on_time;               /* �����׶�ʱ�� */
    USHORT_T off_time;              /* Ϩ��׶�ʱ�� */
    UINT_T total;                   /* ָ����ʱ���ָ���Ĵ��� */
    LED_CALLBACK end_cb;            /* ��˸����ʱִ�еĻص����� */
    UINT_T work_timer;              /* ��˸�����ü�ʱ���� */
} LED_FLASH_T;

/* ����������LED��˸��Ϣ����LED��������LED��Ϣ�����б� (tuya_led.c) */
typedef struct led_manage_s {
    struct led_manage_s *next;
    LED_DRV_T drv_s;
    LED_FLASH_T *flash;             /* led flash related */
    BOOL_T stop_flash_req;          /* ֹͣ��˸���� */
    BOOL_T stop_flash_light;        /* ֹͣ��˸�������״̬ */
} LED_MANAGE_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
/* ��һ��LED��Ϣ�洢�ĵ�ַ��ʵ�ֶ�LED���� */
STATIC LED_MANAGE_T *sg_led_mag_list = NULL;
STATIC tuya_ble_timer_t led_timer;

/***********************************************************
***********************function define**********************
***********************************************************/
STATIC VOID_T __led_timer_cb(VOID_T);

/**
 * @brief led gpio init
 * @param[in] pin: pin number
 * @param[in] active_low: TRUE - active low, FALSE - active high
 * @return none
 */
/* ���Ĳ�����дLED���ų�ʼ��������LEDע�ắ�� (tuya_led.c) */
STATIC VOID_T __led_gpio_init(_IN CONST gpio_pin_e port, _IN CONST BOOL_T active_low)
{
    tuya_gpio_init(port, FALSE, active_low);
}

/**
 * @brief tuya create led handle
 * @param[in] pin: led pin
 * @param[in] active_low: TURE - active low, FALSE - active high
 * @param[out] handle: led handle
 * @return LED_RET
 */
LED_RET tuya_create_led_handle(_IN CONST gpio_pin_e pin, _IN CONST UCHAR_T active_low, _OUT LED_HANDLE *handle)
{
    /* �������δָ���򷵻ش������ */
    if (NULL == handle) {
        return LED_ERR_INVALID_PARM;
    }

    /* Ϊled_mag����ռ䲢��ʼ��������ʧ���򷵻ش�����Ϣ */
    LED_MANAGE_T *led_mag = (LED_MANAGE_T *)tuya_ble_malloc(SIZEOF(LED_MANAGE_T));
    if (NULL == led_mag) {
        return LED_ERR_MALLOC_FAILED;
    }

    /* �����û����õ���Ч��ƽ�����Ž��г�ʼ�� (ע��ʱĬ�ϲ�����) */
    __led_gpio_init(pin, active_low);

    /* ��¼�û����õ�LED��Ϣ���������LED�����б�ͬʱ���洢��ַ */
    led_mag->drv_s.pin = pin;
    led_mag->drv_s.active_low = active_low;
    *handle = (LED_HANDLE)led_mag;

    if (sg_led_mag_list) {
        led_mag->next = sg_led_mag_list;
        sg_led_mag_list = led_mag;
    } else {
        sg_led_mag_list = led_mag;
        tuya_ble_timer_create(&led_timer, LED_TIMER_VAL_MS, TUYA_BLE_TIMER_REPEATED, __led_timer_cb);
        tuya_ble_timer_start(led_timer);
    }

    return LED_OK;
}

/**
 * @brief led reset
 * @param[in] none
 * @return LED_RET
 */
LED_RET tuya_led_reset(VOID_T)
{
    LED_MANAGE_T *led_mag_tmp = sg_led_mag_list;
    if (NULL == led_mag_tmp) {
        return LED_ERR_INVALID_PARM;
    }
    while (led_mag_tmp) {
        __led_gpio_init(led_mag_tmp->drv_s.pin, led_mag_tmp->drv_s.active_low);
        led_mag_tmp = led_mag_tmp->next;
    }
    tuya_ble_timer_delete(led_timer);
    tuya_ble_timer_create(&led_timer, LED_TIMER_VAL_MS, TUYA_BLE_TIMER_REPEATED, __led_timer_cb);
    tuya_ble_timer_start(led_timer);
    return LED_OK;
}

/**
 * @brief set led light on or off
 * @param[in] drv_s: pin and active level
 * @param[in] on_off: TURE - light on, FALSE - light off
 * @return none
 */
/* ��һ������дLED������ƺ��� (tuya_led.c) */
STATIC VOID_T __set_led_light(_IN CONST LED_DRV_T drv_s, _IN CONST BOOL_T on_off)
{
    if (drv_s.active_low) {
        tuya_gpio_write(drv_s.pin, !on_off);
        //hal_gpio_write(TY_GPIO_14, !on_off);
    } else {
        tuya_gpio_write(drv_s.pin, on_off);
        //hal_gpio_write(TY_GPIO_14, on_off);
    }
    
}

/**
 * @brief set led light on or off
 * @param[in] handle: led handle
 * @param[in] on_off: TURE - light on, FALSE - light off
 * @return LED_RET
 */
/* �ڰ˲����޸�LED������ƺ��� (���ܻ������˸δ����ʱ�����˸ú��������) (tuya_led.c) */
LED_RET tuya_set_led_light(_IN CONST LED_HANDLE handle, _IN CONST BOOL_T on_off)
{
    LED_MANAGE_T *led_mag = (LED_MANAGE_T *)handle;
    if (led_mag->flash != NULL) {
        led_mag->stop_flash_req = TRUE;
        led_mag->stop_flash_light = on_off;
    } else {
        __set_led_light(led_mag->drv_s, on_off);
    }
    return LED_OK;
}

/**
 * @brief get led flash start light
 * @param[in] type: led flash type
 * @return TRUE - light on, FALSE - light off
 */
/* ���Ĳ�����д���º������ڽ����û����ã�������˸����ʱʹ�� (tuya_led.c) */
/* ��ȡ��˸��ʼʱ������״̬��1-����0-�� */
STATIC BOOL_T __get_led_flash_sta_light(_IN CONST LED_FLASH_TYPE_E type)
{
    BOOL_T ret = TRUE;
    switch (type) {
    case LFT_STA_ON_END_ON:
    case LFT_STA_ON_END_OFF:
        ret = TRUE;
        break;
    case LFT_STA_OFF_END_ON:
    case LFT_STA_OFF_END_OFF:
        ret = FALSE;
        break;
    default:
        break;
    }
    return ret;
}

/**
 * @brief get led flash end light
 * @param[in] type: led flash type
 * @return TRUE - light on, FALSE - light off
 */
/* ��ȡ��˸�����������״̬��1-����0-�� */
STATIC BOOL_T __get_led_flash_end_light(_IN CONST LED_FLASH_TYPE_E type)
{
    BOOL_T ret = TRUE;
    switch (type) {
    case LFT_STA_ON_END_ON:
    case LFT_STA_OFF_END_ON:
        ret = TRUE;
        break;
    case LFT_STA_ON_END_OFF:
    case LFT_STA_OFF_END_OFF:
        ret = FALSE;
        break;
    default:
        break;
    }
    return ret;
}

/**
 * @brief set led flash in time type and different flash interval
 * @param[in] handle: led handle
 * @param[in] mode: flash mode
 * @param[in] type: flash type
 * @param[in] on_time: light on time (ms)
 * @param[in] off_time: light off time (ms)
 * @param[in] total: it means flash total time (ms) when led mode is LFM_SPEC_TIME, or means flash total count when led mode is LFM_SPEC_COUNT
 * @param[in] flash_end_cb: flash end callback function
 * @return LED_RET
 */
/* ���岽����дLED��˸���ú��� (tuya_led.c) */
LED_RET tuya_set_led_flash(_IN CONST LED_HANDLE handle, _IN CONST LED_FLASH_MODE_E mode, _IN CONST LED_FLASH_TYPE_E type, _IN CONST USHORT_T on_time, _IN CONST USHORT_T off_time, _IN CONST UINT_T total, _IN CONST LED_CALLBACK flash_end_cb)
{
    LED_MANAGE_T *led_mag = (LED_MANAGE_T *)handle;
    led_mag->stop_flash_req = FALSE;
    if (led_mag->flash == NULL) {
        LED_FLASH_T *led_flash = (LED_FLASH_T *)tuya_ble_malloc(SIZEOF(LED_FLASH_T));
        if (NULL == led_flash) {
            return LED_ERR_MALLOC_FAILED;
        }
        led_mag->flash = led_flash;
    }
    led_mag->flash->mode = mode;
    led_mag->flash->type = type;
    led_mag->flash->on_time = on_time;
    led_mag->flash->off_time = off_time;
    led_mag->flash->total = total;
    led_mag->flash->work_timer = 0;
    led_mag->flash->end_cb = flash_end_cb;
    __set_led_light(led_mag->drv_s, __get_led_flash_sta_light(type));
    return LED_OK;
}

/**
 * @brief led flash process
 * @param[inout] led_mag: led management
 * @return none
 */
STATIC VOID_T __led_flash_proc(_INOUT LED_MANAGE_T *led_mag)
{
    BOOL_T one_cycle_flag = FALSE;
    UINT_T sum_time;
    BOOL_T start_light;
    USHORT_T start_time;

    /* ������˸���� */
    sum_time = led_mag->flash->on_time + led_mag->flash->off_time;
    start_light = __get_led_flash_sta_light(led_mag->flash->type);
    start_time = (start_light) ? led_mag->flash->on_time : led_mag->flash->off_time;

    /* ��˸�����Դ���ʵ�ְ���ָ��ʱ�������Ϩ�� */
    led_mag->flash->work_timer += LED_TIMER_VAL_MS;
    if (led_mag->flash->work_timer >= sum_time) {
        led_mag->flash->work_timer -= sum_time;
        __set_led_light(led_mag->drv_s, start_light);
        one_cycle_flag = TRUE;
    } else if (led_mag->flash->work_timer >= start_time) {
        __set_led_light(led_mag->drv_s, !start_light);
    } else {
        ;
    }

    /* ��˸����ʱ/��������˸��ʽΪ����Զ��˸��ʱ������ */
    if (led_mag->flash->mode == LFM_FOREVER) {
        return;
    }
    if (led_mag->flash->mode == LFM_SPEC_TIME) {
        if (led_mag->flash->total > LED_TIMER_VAL_MS) {
            led_mag->flash->total -= LED_TIMER_VAL_MS;
        } else {
            led_mag->flash->total = 0;
        }
    } else if (led_mag->flash->mode == LFM_SPEC_COUNT) {
        if (one_cycle_flag) {
            if (led_mag->flash->total > 0) {
                led_mag->flash->total--;
            }
        }
    } else {
        ;
    }

    /* ��˸�������� */
    if (led_mag->flash->total == 0) {
        if (led_mag->flash->end_cb != NULL) {
            led_mag->flash->end_cb();
        }
        /* ����ֹͣ��˸���󣬲�����ֹͣ�������״̬ */
        led_mag->stop_flash_req = TRUE;
        led_mag->stop_flash_light = __get_led_flash_end_light(led_mag->flash->type);
    }
}

/**
 * @brief led timer callback
 * @param[in] none
 * @return none
 */
/* ���߲�����дLED��ʱ������ (tuya_led.c) */
STATIC VOID_T __led_timer_cb(VOID_T)
{
    /* ��ȡLED��Ϣ�����б���LEDע���򷵻� */
    LED_MANAGE_T *led_mag_tmp = sg_led_mag_list;
    if (NULL == led_mag_tmp) {
        return;
    }
    /* ѭ������ÿ��LED */
    while (led_mag_tmp) {
        if (led_mag_tmp->stop_flash_req) {
            __set_led_light(led_mag_tmp->drv_s, led_mag_tmp->stop_flash_light);
            tuya_ble_free((UCHAR_T *)led_mag_tmp->flash);
            led_mag_tmp->flash = NULL;
            led_mag_tmp->stop_flash_req = FALSE;
        }
        /* �����˸����δ�����򲻴��� */
        if (NULL != led_mag_tmp->flash) {
            __led_flash_proc(led_mag_tmp);
        }
        /* ������һ��LED��Ϣ */
        led_mag_tmp = led_mag_tmp->next;
    }
		//TY_PRINTF("__led_timer_cb");
    //return 0;
}

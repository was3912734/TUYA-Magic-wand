/**
 * @file tuya_key.c
 * @author lifan
 * @brief key driver source file
 * @version 1.0.0
 * @date 2021-09-23
 *
 * @copyright Copyright (c) tuya.inc 2021
 *
 */

#include "tuya_key.h"
//#include <key.h>
#include "tuya_ble_mem.h"
#include "tuya_ble_port.h"

/***********************************************************
************************micro define************************
***********************************************************/
#define KEY_SCAN_CYCLE_MS       10/* 扫描周期 */
#define KEY_PRESS_SHORT_TIME    50/* 短按确认时间 */

/***********************************************************
***********************typedef define***********************
***********************************************************/
/*定义用来存储按键状态的结构体类型*/
typedef struct {
    BOOL_T cur_stat;
    BOOL_T prv_stat;
    UINT_T cur_time;
    UINT_T prv_time;
} KEY_STATUS_T;

/* 定义用于按键信息管理的结构体类型和该类型的指针 */
typedef struct key_manage_s {
    struct key_manage_s *next;
    KEY_DEF_T *key_def_s;
    KEY_STATUS_T key_status_s;
} KEY_MANAGE_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
STATIC KEY_MANAGE_T *sg_key_mag_list = NULL;
STATIC tuya_ble_timer_t key_timer;

/***********************************************************
***********************function define**********************
***********************************************************/
//编写10ms处理函数 
STATIC VOID_T __key_timer_cb(VOID_T);

/**
 * @brief key gpio init
 * @param[in] pin: pin number
 * @param[in] active_low: TRUE - active low, FALSE - active high
 * @return none
 */
//编写按键注册函数，包括对按键的初始化工作
STATIC VOID_T __key_gpio_init(_IN CONST gpio_pin_e port, _IN CONST BOOL_T active_low)//ok
{
    tuya_gpio_init(port, TRUE, active_low);//1
	  //TY_PRINTF("__key_gpio_init %d",active_low);
}

/**
 * @brief key register 在按键注册函数中创建定时器
 * @param[in] key_def: user key define
 * @return KEY_RET
 */
KEY_RET tuya_reg_key(_IN KEY_DEF_T *key_def)
{
    /* 检查是否定义了回调函数，未定义则返回错误信息*/
    if (key_def->key_cb == NULL) {
        return KEY_ERR_CB_UNDEFINED;
    }

    /* 为key_mag分配空间并初始化，分配失败则返回错误信息 */
    KEY_MANAGE_T *key_mag = (KEY_MANAGE_T *)tuya_ble_malloc(SIZEOF(KEY_MANAGE_T));
    if (NULL == key_mag) {
        return KEY_ERR_MALLOC_FAILED;
    }

    /* gpio init 根据用户设置的有效电平对引脚进行初始化 */
    __key_gpio_init(key_def->port, key_def->active_low);//1
		//TY_PRINTF("__key_gpio_init %d",key_def->active_low);
    

    /* update key manage list */
    /* 记录用户设置的按键信息，并存放至按键管理列表 */
    key_mag->key_def_s = key_def;
    if (sg_key_mag_list) {
    	key_mag->next = sg_key_mag_list;
        sg_key_mag_list = key_mag;
    } else {
        sg_key_mag_list = key_mag;
        tuya_ble_timer_create(&key_timer, KEY_SCAN_CYCLE_MS, TUYA_BLE_TIMER_REPEATED, __key_timer_cb);
        tuya_ble_timer_start(key_timer);
    }

    return KEY_OK;
}

/**
 * @brief key reset
 * @param[in] none
 * @return KEY_RET
 */

KEY_RET tuya_key_reset(VOID_T)
{
    KEY_MANAGE_T *key_mag_tmp = sg_key_mag_list;
    if (NULL == key_mag_tmp) {
        return KEY_ERR_CB_UNDEFINED;
    }
    while (key_mag_tmp) {
        __key_gpio_init(key_mag_tmp->key_def_s->port, key_mag_tmp->key_def_s->active_low);
        key_mag_tmp = key_mag_tmp->next;
    }
    tuya_ble_timer_delete(key_timer);
    tuya_ble_timer_create(&key_timer, KEY_SCAN_CYCLE_MS, TUYA_BLE_TIMER_REPEATED, __key_timer_cb);
    tuya_ble_timer_start(key_timer);
    return KEY_OK;
}


/**
 * @brief get the real-time status of the key
 * @param[in] port: key port
 * @param[in] active_low: TURE - active low, FALSE - active high
 * @return key_stat: TRUE - press, FALSE - release
 */
//编写用于更新单个按键状态的相关函数
/* 获取按键实时状态，1-按压，0-释放 */
STATIC BOOL_T __get_key_stat(_IN CONST gpio_pin_e port, _IN CONST UCHAR_T active_low)
{
    BOOL_T key_stat;
    if (active_low) {
        key_stat = tuya_gpio_read(port) == 0 ? TRUE : FALSE;
			  //key_stat = hal_gpio_read(port) == 0 ? TRUE : FALSE;
    } else {
        key_stat = tuya_gpio_read(port) == 0 ? FALSE : TRUE;
			  //key_stat = hal_gpio_read(port) == 0 ? TRUE : FALSE;
    }
    return key_stat;
}

/**
 * @brief update key status
 * @param[inout] key_mag: key manage information
 * @return none
 */
/* 更新按键状态 */
STATIC VOID_T __update_key_status(_INOUT KEY_MANAGE_T *key_mag)
{
    BOOL_T key_stat;
    /* 保存上次状态*/
    key_mag->key_status_s.prv_stat = key_mag->key_status_s.cur_stat;
    key_mag->key_status_s.prv_time = key_mag->key_status_s.cur_time;
    /* 获取实时状态*/
    key_stat = __get_key_stat(key_mag->key_def_s->port, key_mag->key_def_s->active_low);
    /* 更新本次状态*/
    if (key_stat != key_mag->key_status_s.cur_stat) {
        key_mag->key_status_s.cur_stat = key_stat;
        key_mag->key_status_s.cur_time = 0;
    } else {
        key_mag->key_status_s.cur_time += KEY_SCAN_CYCLE_MS;
    }
}

 //编写用于判断单个按键事件的相关函数
/**
 * @brief is key press over time判断按键保持按压状态时间是否到达了over_time
 * @param[in] key_status_s: key status
 * @param[in] over_time: over time used for judgment
 * @return TRUE or FALSE
 */
STATIC BOOL_T __is_key_press_over_time(_IN CONST KEY_STATUS_T key_status_s, _IN CONST UINT_T over_time)
{
    if (key_status_s.cur_stat == TRUE) {
        if ((key_status_s.cur_time >= over_time) &&
            (key_status_s.prv_time < over_time)) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief is key from release to release over time and less time
 * @param[in] key_status_s: key status
 * @param[in] over_time: over time used for judgment
 * @param[in] less_time: less time used for judgment
 * @return TRUE or FALSE
 */
//判断按键从开始被按压到被释放经过的时间是否到达了over_time，并且少于less_time
STATIC BOOL_T __is_key_release_to_release_over_time_and_less_time(_IN CONST KEY_STATUS_T key_status_s, _IN CONST UINT_T over_time, _IN CONST UINT_T less_time)
{
    if ((key_status_s.prv_stat == TRUE) &&
        (key_status_s.cur_stat == FALSE)) {
        if ((key_status_s.prv_time >= over_time) &&
            (key_status_s.prv_time < less_time)) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief detect and handle key event
 * @param[inout] key_mag: key manage information
 * @return none
 */
/* 判断与处理按键事件 */
STATIC VOID_T __detect_and_handle_key_event(_INOUT KEY_MANAGE_T *key_mag)
{
    KEY_PRESS_TYPE_E type;
    BOOL_T time_exchange;
    UINT_T long_time1, long_time2;

    /* 比较用户设置的长按时间1个长按时间2的大小，并标记是否交换 */
    if (key_mag->key_def_s->long_press_time2 >= key_mag->key_def_s->long_press_time1) {
        long_time1 = key_mag->key_def_s->long_press_time1;
        long_time2 = key_mag->key_def_s->long_press_time2;
        time_exchange = FALSE;
    } else {
        long_time1 = key_mag->key_def_s->long_press_time2;
        long_time2 = key_mag->key_def_s->long_press_time1;
        time_exchange = TRUE;
    }
    /* 判断按键状态，标记事件类型并跳转到key_event(根据长按事件设置情况使用对应的判断方式)*/
    if ((long_time2 != 0) && (long_time1 != 0)) {
        if (__is_key_press_over_time(key_mag->key_status_s, long_time2)) {
            type = LONG_PRESS_FOR_TIME2;
            goto KEY_EVENT;
        }
        if (__is_key_release_to_release_over_time_and_less_time(key_mag->key_status_s, long_time1, long_time2)) {
            type = LONG_PRESS_FOR_TIME1;
            goto KEY_EVENT;
        }
        if (__is_key_release_to_release_over_time_and_less_time(key_mag->key_status_s, KEY_PRESS_SHORT_TIME, long_time1)){
            type = SHORT_PRESS;
            goto KEY_EVENT;
        }
    } else if ((long_time2 != 0) && (long_time1 == 0)) {
        if (__is_key_press_over_time(key_mag->key_status_s, long_time2)) {
            type = LONG_PRESS_FOR_TIME2;
            goto KEY_EVENT;
        }
        if (__is_key_release_to_release_over_time_and_less_time(key_mag->key_status_s, KEY_PRESS_SHORT_TIME, long_time2)){
            type = SHORT_PRESS;
            goto KEY_EVENT;
        }
    } else if ((long_time2 == 0) && (long_time1 != 0)) {
        if (__is_key_press_over_time(key_mag->key_status_s, long_time1)) {
            type = LONG_PRESS_FOR_TIME1;
            goto KEY_EVENT;
        }
        if (__is_key_release_to_release_over_time_and_less_time(key_mag->key_status_s, KEY_PRESS_SHORT_TIME, long_time1)){
            type = SHORT_PRESS;
            goto KEY_EVENT;
        }
    } else {
        if (__is_key_press_over_time(key_mag->key_status_s, KEY_PRESS_SHORT_TIME)) {
            type = SHORT_PRESS;
            goto KEY_EVENT;
        }
    }
    return;
    /* handle key event 处理按键事件*/
KEY_EVENT:
    /* 如果在判断前进行了时间参数交换，则将标记的事件类型进行交换 */
    if (time_exchange) {
        /* exchanged the event type  */
        if (type == LONG_PRESS_FOR_TIME2) {
            type = LONG_PRESS_FOR_TIME1;
        } else if (type == LONG_PRESS_FOR_TIME1) {
            type = LONG_PRESS_FOR_TIME2;
        } else {
            ;
        }
    }
    /* 执行用户设置的回调函数*/
    key_mag->key_def_s->key_cb(type);
}

/**
 * @brief key timer callback 编写10ms处理函数 
 * @param[in] none
 * @return none
 */
STATIC VOID_T __key_timer_cb(VOID_T)
{
	  //TY_PRINTF("__key_timer_cb");
	  //获取按键信息管理列表，无按键注册则返回
    KEY_MANAGE_T *key_mag_tmp = sg_key_mag_list;
    if (NULL == key_mag_tmp) {
			  TY_PRINTF("__key_timer_cb0");
        return ;
    }
		//循环处理每个按键
    while (key_mag_tmp) {
        __update_key_status(key_mag_tmp);/* 更新按键状态 */
        __detect_and_handle_key_event(key_mag_tmp);/* 判断并处理按键事件 */
        key_mag_tmp = key_mag_tmp->next;/* 加载下一个按键信息 */
			  //TY_PRINTF("__key_timer_cb1");
    }
    //return 0;
}

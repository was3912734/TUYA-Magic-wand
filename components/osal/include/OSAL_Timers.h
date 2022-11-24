/**************************************************************************************************
    Filename:       OSAL_Timers.h
    Revised:
    Revision:

    Description:    This file contains the OSAL Timer definition and manipulation functions.



**************************************************************************************************/

#ifndef OSAL_TIMERS_H
#define OSAL_TIMERS_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
    INCLUDES
*/

/*********************************************************************
    MACROS
*/

/*********************************************************************
    CONSTANTS
    the unit is chosen such that the 320us tick equivalent can fit in
    32 bits.
*/
#define OSAL_TIMERS_MAX_TIMEOUT 0x28f5c28e /* unit is ms*/


/*********************************************************************
    TYPEDEFS
*/
typedef union
{
    uint32 time32;
    uint16 time16[2];
    uint8 time8[4];
} osalTime_t;

typedef struct
{
    void*   next;
    osalTime_t timeout;
    uint16 event_flag;
    uint8  task_id;
    uint32 reloadTimeout;
} osalTimerRec_t;


/*********************************************************************
    GLOBAL VARIABLES
*/

/*********************************************************************
    FUNCTIONS
*/

/*
    OSAL定时器系统的初始化.
*/

extern osalTimerRec_t* osalFindTimer( uint8 task_id, uint16 event_flag );

extern void osalTimerInit( void );

/*
    设定一个计时器
*/
extern uint8 osal_start_timerEx( uint8 task_id, uint16 event_id, uint32 timeout_value );

/*
    设置一个自动重新加载的计时器
*/
extern uint8 osal_start_reload_timer( uint8 taskID, uint16 event_id, uint32 timeout_value );

/*
    Stop a Timer
*/
extern uint8 osal_stop_timerEx( uint8 task_id, uint16 event_id );

/*
    获取计时器的滴答数
*/
extern uint32 osal_get_timeoutEx( uint8 task_id, uint16 event_id );

/*
    模拟定时器中断服务程序
*/

extern void osal_timer_ISR( void );

/*
    
    调整计时器
*/
extern void osal_adjust_timers( void );

/*
    
    更新计时器表
*/
extern void osalTimerUpdate( uint32 updateTime );

/*
    计数活动计时器
*/
extern uint8 osal_timer_num_active( void );

/*
    将硬件定时器中断设置为睡眠模式
    These functions should only be called in OSAL_PwrMgr.c
*/
extern void osal_sleep_timers( void );
extern void osal_unsleep_timers( void );

/*
    读取系统时钟-返回毫秒
*/
extern uint32 osal_GetSystemClock( void );

/*
    获取下一个OSAL计时器过期时间。此函数只能在OSAL_PwrMgr.c中调用
*/
extern uint32 osal_next_timeout( void );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OSAL_TIMERS_H */

/**
 * @file tuya_pwr_mgmt_nRF52832.c
 * @author lifan
 * @brief tuya power management for nRF52832
 * @version 1.0.0
 * @date 2021-11-05
 *
 * @copyright Copyright (c) tuya.inc 2021
 *
 */

#include "tuya_pwr_mgmt.h"
//#include "ty_pin.h"
#include "tuya_gpio.h"
#include "pwrmgr.h"
//#include "clock.h"
//#include "board.h"
//#include "app_timer.h"
//#include "app_error.h"
//#include "nrf_soc.h"
//#include "nrf_sdm.h"
//#include "bsp_btn_ble.h"

/***********************************************************
************************micro define************************
***********************************************************/
#if 0
/* modify the wake-up button configuration in pca10040.h,
   refer to the following code !!! */
//#define WAKEUP_BUTTONS   GPIO_P02    //开始按键

#define BUTTON_START   5//13
#define BUTTON_1       5//13
#define BUTTON_2       14
#define BUTTON_3       15
#define BUTTON_4       16
#define BUTTON_STOP    16
#define BUTTON_PULL    GPIO_PULL_UP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1 }//, BUTTON_2, BUTTON_3, BUTTON_4 }

#define BSP_BUTTON_0   BUTTON_1
//#define BSP_BUTTON_1   BUTTON_2
//#define BSP_BUTTON_2   BUTTON_3
//#define BSP_BUTTON_3   BUTTON_4
#endif

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define**********************
***********************************************************/
/**
 * @brief putting the chip into sleep mode将芯片置于睡眠模式
 * @param[in] wakeup_pin: wake-up pin唤醒引脚
 * @param[in] cnt: wake-up pin number唤醒密码
 * @return none
 */
VOID_T tuya_enter_sleep_mode(UINT_T *wakeup_pin, UCHAR_T cnt)
{
    //ret_code_t err_code;

    /* prepare wakeup buttons准备唤醒按钮*/
    //err_code = bsp_btn_ble_sleep_mode_prepare();
    //APP_ERROR_CHECK(err_code);
	
    for (UCHAR_T i = 0; i < cnt; i++) {
        //nrf_gpio_cfg_input(wakeup_pin[i], NRF_GPIO_PIN_PULLUP);
        //nrf_gpio_pin_sense_t sense = NRF_GPIO_PIN_SENSE_LOW;
        //nrf_gpio_cfg_sense_set(wakeup_pin[i], sense);
		//hal_gpio_wakeup_set(wakeup_pin[i],NEGEDGE);	  
    }

		//hal_pwrmgr_poweroff(GPIO_P02);
		pwroff_cfg_t pwr_wkp_cfg[]= {{GPIO_P34,NEGEDGE}};
    hal_pwrmgr_poweroff( pwr_wkp_cfg, 1 );


    /* go to system-off mode (this function does not return; wakeup causes a reset) */
    //err_code = sd_power_system_off();
    //APP_ERROR_CHECK(err_code);
    //sd_softdevice_disable();
    //NRF_POWER->SYSTEMOFF = TRUE;
    //nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
        //ty_system_enter_sleep();

}

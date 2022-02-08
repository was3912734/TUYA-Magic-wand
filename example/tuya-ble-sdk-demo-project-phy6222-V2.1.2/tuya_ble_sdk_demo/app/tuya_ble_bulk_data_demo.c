#include "tuya_ble_stdlib.h"
#include "tuya_ble_type.h"
#include "tuya_ble_heap.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_main.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_sdk_version.h"
#include "tuya_ble_utils.h"
#include "tuya_ble_event.h"
#include "tuya_ble_log.h"
#include "tuya_ble_internal_config.h"
#include "tuya_ble_bulk_data_demo.h"
#include "tuya_ble_bulk_data.h"
#include "ty_rtc.h"




/*********************************************************************
 * LOCAL CONSTANT
 */
/**@brief   Macro for defining the test bulk type value.
 *
 * @details The bulk type is used to indicate the specific type of bulk data contained in the device. 
 * For example, a smart bracelet defines two types of data to be stored , one is daily sport data (steps, distance, and calories), 
 * and the other is sleep report data. The two types of data are stored separately in the device. In this application scenario, 
 * the device can distinguish the two types of bulk data by bulk type. For example, a bulk type of 0 indicates daily sport data, 
 * and 1 indicates sleep report data.
 *  
 */
#define TEST_BULK_DATA_TYPE                         1

#define TEST_BULK_DATA_TYPE1_START_ADDR             BOARD_FLASH_BULK_DATA_START_ADDR
#define TEST_BULK_DATA_TYPE1_END_ADDR               (TEST_BULK_DATA_TYPE1_START_ADDR+0x4000)

#if ((TEST_BULK_DATA_TYPE1_START_ADDR>=TUYA_NV_START_ADDR) || (TEST_BULK_DATA_TYPE1_END_ADDR>TUYA_NV_START_ADDR))
#error "Storage Memory overflow!"
#endif


/**@brief   Macro for defining the block size.
 *
 * @details The mobile app reads bulk data in units of blocks. The size of the block is defined by the specific device application. 
 * The maximum currently supported is 512.
 *  
 */
#define TEST_BULK_DATA_TYPE1_BLOCK_SIZE             512   /**< Preferably not more than 512*/

#if (TEST_BULK_DATA_TYPE1_BLOCK_SIZE>=TUYA_BLE_SEND_MAX_DATA_LEN)
#error "TEST_BULK_DATA_TYPE1_BLOCK_SIZE is over!"
#endif

/*********************************************************************
 * LOCAL STRUCT
 */

/*********************************************************************
 * LOCAL VARIABLE
 */
/**@brief   Storage format of bulk data.
 *
 * @details All bulk data must be stored in the format of 'TLD' array ('TLDTLDTLDTLD'), where 'T (Type)' occupies 1 byte, 
 * which is used to define the data format of the following 'LD' ; 'L (Length)' occupies 2 Byte, indicating the length of the following 'Data'; 
 * 'D(Data)', consisting of 'Length' bytes of data. When the 'Type' value is 1 (other values are reserved and undefined), the following 'Data' 
 * content is a 4-byte unix timestamp (unsigned int) + Dp data array, as shown below:
 *  
 */
uint8_t TLD_Example_Data[64] =
{
    0x01,                                                                            /* TYPE:1 */
    0x00,0x3D,                                                                       /* LENGTH: 0x003D = 61*/
    0x5F,0xC0,0xAF,0xDD,                                                             /* UNIX TIME: 0x5FC0AFDD = 1606463453 */
    0x14,0x02,0x00,0x04,0x00,0x00,0x00,0x0A,                                         /* dp id: 20; dp type : value; dp len : 4; dp data : 10 */
    0x15,0x02,0x00,0x04,0x00,0x00,0x00,0x64,                                         /* dp id: 21; dp type : value; dp len : 4; dp data : 100 */
    0x16,0x02,0x00,0x04,0x00,0x00,0x00,0x28,                                         /* dp id: 22; dp type : value; dp len : 4; dp data : 40 */
    0x18,0x02,0x00,0x04,0x00,0x00,0x00,0x32,                                         /* dp id: 24; dp type : value; dp len : 4; dp data : 50 ����*/
    0x19,0x02,0x00,0x04,0x00,0x00,0x00,0x46,                                         /* dp id: 25; dp type : value; dp len : 4; dp data : 70 */
    0x69,0x00,0x00,0x0D,0x14,0x09,0x04,0x01,0x0e,0x00,0x02,0x0f,0x00,0x00,0x10,0x00, /* dp id: 105; dp type : raw; dp len : 18; dp data : raw data */
    0x01,
};
//uint8_t TLD_Example_Data[32] =
//{
//    0x01,                                                                            /* TYPE:1 */
//    0x00,0x3D,                                                                       /* LENGTH: 0x003D = 61*/
//    0x5F,0xC0,0xAF,0xDD,                                                             /* UNIX TIME: 0x5FC0AFDD = 1606463453 */
//    0x18,0x02,0x00,0x04,0x00,0x00,0x00,0x0A,                                         /* dp id: 24; dp type : value; dp len : 4; dp data : 50 */
//    0x69,0x00,0x00,0x0D,0x14,0x09,0x04,0x01,0x0e,0x00,0x02,0x0f,0x00,0x00,0x10,0x00, /* dp id: 105; dp type : raw; dp len : 18; dp data : raw data */
//    0x01,
//};

static uint32_t bulk_data_total_length = 0;
static uint32_t bulk_data_total_crc32 = 0;
static uint32_t bulk_data_total_blocks = 0;

static uint8_t  current_bulk_type_being_read = 0;
static uint16_t current_block_number = 0;
static uint16_t current_block_size = 0;

/*********************************************************************
 * VARIABLE
 */

/*********************************************************************
 * LOCAL FUNCTION
 */




/*********************************************************
FN: 
*/
/**@brief  Function for generate bulk data for testing.
 *
 * @details The device application calls this function to generate bulkdata test data.
 *  
 */
uint32_t tuya_ble_bulk_data_generation(uint32_t timestep, uint8_t* buf, uint32_t size)
{
    uint64_t data_len = sizeof(TLD_Example_Data);
    uint32_t dp_time = 0;
    
    ty_rtc_get_time(&dp_time);
    
    if(buf != NULL) {
        memcpy(TLD_Example_Data, buf, size);
        data_len = size;
        dp_time = (buf[3]<<24) + (buf[4]<<16) + (buf[5]<<8) + buf[6];
    }
    
    if(dp_time == 0) {
        ty_rtc_get_time(&dp_time);
    }
    
    uint32_t write_addr = TEST_BULK_DATA_TYPE1_START_ADDR;
    bulk_data_total_length = 0;
    bulk_data_total_crc32 = 0;

    while(write_addr < TEST_BULK_DATA_TYPE1_END_ADDR) {
        
        if(write_addr % TUYA_NV_ERASE_MIN_SIZE == 0) {
            if(tuya_ble_nv_erase(write_addr, TUYA_NV_ERASE_MIN_SIZE) != TUYA_BLE_SUCCESS) {
                TUYA_APP_LOG_ERROR("tuya_ble_bulk_data_generation ERROR!");
                bulk_data_total_length = 0;
                bulk_data_total_crc32 = 0;
                return 1;
            }
        }

        TLD_Example_Data[3] = dp_time>>24;
        TLD_Example_Data[4] = dp_time>>16;
        TLD_Example_Data[5] = dp_time>>8;
        TLD_Example_Data[6] = dp_time;

        if(tuya_ble_nv_write(write_addr, TLD_Example_Data, data_len) != TUYA_BLE_SUCCESS) {
            TUYA_APP_LOG_ERROR("tuya_ble_bulk_data_generation ERROR!");
            bulk_data_total_length = 0;
            bulk_data_total_crc32 = 0;
            return 1;
        }
        
        write_addr += data_len;
        bulk_data_total_length += data_len;
        bulk_data_total_crc32 = tuya_ble_crc32_compute(TLD_Example_Data, data_len, &bulk_data_total_crc32);
        dp_time -= timestep;

        tuya_ble_device_delay_ms(10);
    }

    bulk_data_total_blocks = (bulk_data_total_length + TEST_BULK_DATA_TYPE1_BLOCK_SIZE - 1) / TEST_BULK_DATA_TYPE1_BLOCK_SIZE;

    TUYA_APP_LOG_DEBUG("tuya_ble_bulk_data_generation OK, TOTAL LENGTH = %d , TOTAL CRC32 = 0x%08x, BLOCK SIZE = %d , BLOCK NUMBERS = %d , TIME = %d",
                       bulk_data_total_length,bulk_data_total_crc32,TEST_BULK_DATA_TYPE1_BLOCK_SIZE,bulk_data_total_blocks, dp_time);
    
    return 0;
}

/*********************************************************
FN: 
*/
static uint16_t get_bulk_data_block_crc16(uint16_t block_number, uint16_t block_data_length)
{
    static uint8_t buf[256];
    uint16_t crc_temp = 0xFFFF;

    if(block_number > (bulk_data_total_blocks - 1)) {
        return 0;
    }

    uint32_t read_addr = TEST_BULK_DATA_TYPE1_START_ADDR + block_number*TEST_BULK_DATA_TYPE1_BLOCK_SIZE;

    uint32_t cnt = block_data_length/256;
    uint32_t remainder = block_data_length%256;

    for(uint32_t i = 0; i<cnt; i++) {
        tuya_ble_nv_read(read_addr, buf, 256);
        crc_temp = tuya_ble_crc16_compute(buf, 256, &crc_temp);
        read_addr += 256;
    }

    if(remainder > 0) {
        tuya_ble_nv_read(read_addr, buf, remainder);
        crc_temp = tuya_ble_crc16_compute(buf, remainder, &crc_temp);
        read_addr += remainder;
    }

    return crc_temp;
}


/*********************************************************
FN: 
*/
/**@brief   Function for handling the bulk data events.
 *
 * @details The bulk data reading steps :
 *
 * 1. The mobile app first reads the bulk data information of the specified type, and triggers the Tuya BLE SDK 
 *    to send the 'TUYA_BLE_BULK_DATA_EVT_READ_INFO' event to the device application. The event data contains the 
 *    type value of the bulk data to be read by the mobile app. The device application calls the 'tuya_ble_bulk_data_response()' 
 *    function to send the total length, total CRC32, block size and other information of the bulk data to the mobile app.
 *
 * 2. The mobile app starts the bulk data reading process according to the data information returned by the device application. 
 *    First trigger the Tuya BLE SDK to send the 'TUYA_BLE_BULK_DATA_EVT_READ_BLOCK' event to request the data information of 
 *    the first block, including block number (starting from 0), block size, and CRC16 of the block data. The device application 
 *    calls the'tuya_ble_bulk_data_response()' function to reply to the block data information.
 * 3. After sending the block data information to the mobile app, the Tuya BLE SDK will automatically send the'TUYA_BLE_BULK_DATA_EVT_SEND_DATA' 
 *    event to the device application. The event data contains the block number to be read. The device application calls the'tuya_ble_bulk_data_response()' 
 *    function to send the block data .
 * 4. After sending the block data information to the mobile app, the Tuya BLE SDK will automatically send the'TUYA_BLE_BULK_DATA_EVT_SEND_DATA' 
 *    event to the device application. The event data contains the block number to be read. The device application calls the'tuya_ble_bulk_data_response()' 
 *    function to send the block data.
 * 5. After reading all the blocks, the mobile app will verify the data. When the verification is completed, all the dp point data in it will be parsed and 
 *    uploaded to the cloud. When the upload is complete, the Tuya BLE SDK will be triggered to send the'TUYA_BLE_BULK_DATA_EVT_ERASE' event to the device 
 *    application. The event data contains The type value of bulk data to be erased, after the device application erases the data, 
 * 	  call the'tuya_ble_bulk_data_response()' function to send the erase result.
 * 6. If the device defines multiple types of bulk data, the mobile app will initiate the process of reading bulk data of other types.
 *
 * @note The application must call this function where it receives the @ref TUYA_BLE_CB_EVT_BULK_DATA event. 
 *
 * @param[in] p_data     Event data received from the SDK.
 * 
 */
void tuya_ble_bulk_data_demo_handler(tuya_ble_bulk_data_request_t* p_data)
{
    uint8_t rsp_flag = 1;
    tuya_ble_bulk_data_response_t res_data = {0};

    res_data.evt = p_data->evt;

    switch(p_data->evt)
    {
        case TUYA_BLE_BULK_DATA_EVT_READ_INFO: {
            if(p_data->bulk_type != TEST_BULK_DATA_TYPE) { //Our test has only one type of bulk data.
                res_data.bulk_type = p_data->bulk_type;
                res_data.params.bulk_info_res_data.status = 1;  //Invalid type value.
                TUYA_APP_LOG_ERROR("Invalid bulk type = %d ",p_data->bulk_type);
            } else {
                current_bulk_type_being_read = p_data->bulk_type;
                
                res_data.bulk_type = p_data->bulk_type;
                res_data.params.bulk_info_res_data.status = 0;
                res_data.params.bulk_info_res_data.flag = 0; // Currently only supports 0 and must be 0.
                res_data.params.bulk_info_res_data.bulk_data_length = bulk_data_total_length;
                res_data.params.bulk_info_res_data.bulk_data_crc = bulk_data_total_crc32;
                res_data.params.bulk_info_res_data.block_data_length = TEST_BULK_DATA_TYPE1_BLOCK_SIZE;
            }
        } break;

        case TUYA_BLE_BULK_DATA_EVT_READ_BLOCK: {
            //The bulk data type to be read is not the type specified by the 'TUYA_BLE_BULK_DATA_EVT_READ_INFO' event.
            if(p_data->bulk_type != current_bulk_type_being_read) {
                res_data.bulk_type = p_data->bulk_type;
                res_data.params.block_res_data.status = 1;  //Invalid type value.
                TUYA_APP_LOG_ERROR("Invalid bulk type = %d ",p_data->bulk_type);
            } else {
                current_block_number = p_data->params.block_data_req_data.block_number;
                
                res_data.bulk_type = p_data->bulk_type;
                if(current_block_number > (bulk_data_total_blocks-1)) {
                    res_data.params.block_res_data.status = 2;
                    TUYA_APP_LOG_ERROR("Invalid bulk block number,received block number = %d total blocks = %d", current_block_number, bulk_data_total_blocks);
                } else {
                    res_data.params.block_res_data.status = 0;
                    res_data.params.block_res_data.block_number = current_block_number;

                    current_block_size = (current_block_number < (bulk_data_total_blocks - 1)) ?
                                         TEST_BULK_DATA_TYPE1_BLOCK_SIZE : (bulk_data_total_length - current_block_number*TEST_BULK_DATA_TYPE1_BLOCK_SIZE); //The data length of the last block may be less than TEST_BULK_DATA_TYPE0_BLOCK_SIZE.

                    res_data.params.block_res_data.block_data_length = current_block_size;
                    res_data.params.block_res_data.max_packet_data_length = TEST_BULK_DATA_TYPE1_BLOCK_SIZE;
                    res_data.params.block_res_data.block_data_crc16 = get_bulk_data_block_crc16(current_block_number,current_block_size);

                    TUYA_APP_LOG_DEBUG("Read block data : block number = %d, block size = %d ,block data crc16 = 0x%04x",
                                       current_block_number,current_block_size,res_data.params.block_res_data.block_data_crc16);
                }
            }
        } break;

        case TUYA_BLE_BULK_DATA_EVT_SEND_DATA: {
            //The bulk data type to be read is not the type specified by the 'TUYA_BLE_BULK_DATA_EVT_READ_INFO' event.
            if(p_data->bulk_type != current_bulk_type_being_read) {
                TUYA_APP_LOG_ERROR("Invalid bulk type = %d ,not start send bulk data.",p_data->bulk_type);
            } else {
                current_block_number = p_data->params.send_data_req_data.block_number;
                current_block_size = (current_block_number < (bulk_data_total_blocks - 1)) ?
                                     TEST_BULK_DATA_TYPE1_BLOCK_SIZE : (bulk_data_total_length - current_block_number * TEST_BULK_DATA_TYPE1_BLOCK_SIZE); //The data length of the last block may be less than TEST_BULK_DATA_TYPE0_BLOCK_SIZE.

                if(current_block_size == 0) {
                    TUYA_APP_LOG_ERROR("The current_block_total_packets is 0,not start send bulk data.");
                    rsp_flag = 0;
                } else {
                    res_data.bulk_type = p_data->bulk_type;

                    if(current_block_size > TEST_BULK_DATA_TYPE1_BLOCK_SIZE) {
                        current_block_size = TEST_BULK_DATA_TYPE1_BLOCK_SIZE;
                    }

                    uint8_t* p_buf = (uint8_t *)tuya_ble_malloc(current_block_size);

                    if(p_buf) {
                        res_data.params.send_res_data.current_block_number = current_block_number;
                        res_data.params.send_res_data.current_block_length = current_block_size;
                        uint32_t read_addr = TEST_BULK_DATA_TYPE1_START_ADDR + current_block_number * TEST_BULK_DATA_TYPE1_BLOCK_SIZE;

                        tuya_ble_nv_read(read_addr,p_buf,current_block_size);
                        
                        uint16_t crc16_temp = tuya_ble_crc16_compute(p_buf, current_block_size, NULL);
                        
                        TUYA_APP_LOG_DEBUG("Current block data crc for send : 0x%04x",crc16_temp);

                        res_data.params.send_res_data.p_current_block_data = p_buf;
                    } else {
                        TUYA_APP_LOG_ERROR("tuya_ble_bulk_data_demo_handler malloc failed ,Send termination.");
                        rsp_flag = 0;
                    }
                }
            }
        } break;

        case TUYA_BLE_BULK_DATA_EVT_ERASE: {
            //The bulk data type to be erased is not the type specified by the 'TUYA_BLE_BULK_DATA_EVT_READ_INFO' event.
            if(p_data->bulk_type != current_bulk_type_being_read) {
                res_data.bulk_type = p_data->bulk_type;
                res_data.params.erase_res_data.status = 1;
            } else {
                res_data.bulk_type = p_data->bulk_type;
                res_data.params.erase_res_data.status = 0;

                /** Erase the corresponding bulk data according to the type. */
                bulk_data_total_length = 0;
                bulk_data_total_blocks = 0;
            }
        } break;

        default: {
            rsp_flag = 0;
        } break;
    };

    if(rsp_flag) {
        tuya_ble_bulk_data_response(&res_data);

        if(res_data.evt == TUYA_BLE_BULK_DATA_EVT_SEND_DATA) {
            tuya_ble_free(res_data.params.send_res_data.p_current_block_data);
			TUYA_APP_LOG_DEBUG("Send block data : block number = %d, block size = %d",
                                   res_data.params.send_res_data.current_block_number,res_data.params.send_res_data.current_block_length,res_data.params.send_res_data);
        }
    }
}

















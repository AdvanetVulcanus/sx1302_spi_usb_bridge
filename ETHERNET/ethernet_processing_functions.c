#include <stdio.h>
#include "socket.h"
#include "wizchip_conf.h"
#include "ethernet_poc.h"





/**
 * @brief Decode incomming message
 *
 *  This function needs to be called every time new data are received to the socket.
 *  is_new_frame needs to be set in regards to function output and timeout on communication.
 *
 * @param is_new_frame  true if the buffer is a new frame
 * @param buffer        buffer to decode
 * @param size          size of the buffer
 * @return true         current frame is completely decoded
 * @return false        current frame is waiting for more bytes
 */
bool eth_usb__process_buffer( const bool is_new_frame, const uint8_t* payload, const uint16_t payload_len )
{
    static uint16_t   rx_car_idx = 0;
    static uint16_t   rx_msg_size;
    static order_id_t rx_msg_cmd_id;
    static uint8_t    rx_msg_id;

    bool     ret = false;
    uint16_t i;

    if( is_new_frame == true )
    {
        // Only usefull when timeout on com
        rx_car_idx = 0;
    }

    for( i = 0; i < payload_len; i++ )
    {
        ret = false;
        switch( rx_car_idx )
        {
        case CMD_OFFSET__ID:
            rx_msg_id = payload[i];
            break;

        case CMD_OFFSET__SIZE_MSB:
            rx_msg_size = payload[i] << 8;
            break;

        case CMD_OFFSET__SIZE_LSB:
            rx_msg_size += payload[i];
            if( rx_msg_size > MAX_SIZE_COMMAND )
            {
                rx_car_idx = 0;

                // Size is incorrect --> force a command error an try to recover
                prepare_ack( rx_msg_id, ORDER_ID__CMD_ERROR, CMD_ERROR_SIZE );
                return false;
            }
            break;

        case CMD_OFFSET__CMD:
            rx_msg_cmd_id = payload[i];
            break;

        default:
            rx_msg_buffer[rx_car_idx - CMD_OFFSET__DATA] = payload[i];
            break;
        }
        rx_car_idx += 1;

        if( ( rx_car_idx - CMD_OFFSET__DATA ) == rx_msg_size )
        {
            rx_car_idx = 0;
            ret        = true;
            decode_frame( rx_msg_cmd_id, rx_msg_id, rx_msg_buffer, rx_msg_size );
        }
    }
    return ret;
}





/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

/**
 * @brief Decode op code and call appropriate function
 *
 * @param order      op code
 * @param id         request id
 * @param buffer     data
 * @param buffer_len data len
 */
static void decode_frame( const order_id_t order_id, const uint8_t cmd_id, uint8_t* payload,
                          const uint16_t payload_len )
{
    uint8_t* tx_msg_payload = tx_msg_buffer + CMD_OFFSET__DATA;

    switch( order_id )
    {
    case ORDER_ID__REQ_PING:
        if( payload_len == REQ_PING_SIZE )
        {
            memcpy( tx_msg_payload + ACK_PING__UNIQUE_ID_0, ( uint32_t* ) UID_BASE, 12 );
            memcpy( tx_msg_payload + ACK_PING__VERSION_0, SOFT_VERSION, 9 );

            prepare_ack( cmd_id, ORDER_ID__ACK_PING, ACK_PING_SIZE );
        }
        else
        {
            prepare_ack( cmd_id, ORDER_ID__CMD_ERROR, CMD_ERROR_SIZE );
        }
        break;

    case ORDER_ID__REQ_GET_STATUS:
        if( payload_len == REQ_GET_STATUS_SIZE )
        {
            uint32_t tmp32 = ( uint32_t ) xTaskGetTickCount( );
            write_uint32_msb_to_buffer( tmp32, tx_msg_payload + ACK_GET_STATUS__SYSTEM_TIME_31_24 );

            float   temperature;
            int16_t tmp_int16 = 0xFFFF;  // default value - no temperature

            if( temperature_task__get_temperature_celsius( &temperature ) == true )
            {
                // temperature is x100 to avoid float
                tmp_int16 = ( int16_t )( temperature * 100.0 );
            }

            write_int16_msb_to_buffer( tmp_int16, tx_msg_payload + ACK_GET_STATUS__TEMPERATURE_15_8 );

            prepare_ack( cmd_id, ORDER_ID__ACK_GET_STATUS, ACK_GET_STATUS_SIZE );
        }
        else
        {
            prepare_ack( cmd_id, ORDER_ID__CMD_ERROR, CMD_ERROR_SIZE );
        }
        break;

    case ORDER_ID__REQ_BOOTLOADER_MODE:
        if( payload_len == REQ_BOOTLOADER_MODE_SIZE )
        {
            prepare_ack( cmd_id, ORDER_ID__ACK_BOOTLOADER_MODE, ACK_BOOTLOADER_MODE_SIZE );
            vTaskDelay( 200 );  // Wait a few ms to send the message over usb

            device__set_bootloader( );
        }
        else
        {
            prepare_ack( cmd_id, ORDER_ID__CMD_ERROR, CMD_ERROR_SIZE );
        }
        break;

    case ORDER_ID__REQ_RESET:
        if( payload_len == REQ_RESET_SIZE )
        {
            bool reset_done = false;
            if( ( reset_type_t ) payload[REQ_RESET__TYPE] == RESET_TYPE__GTW )
            {
                reset_done = true;
            }

            tx_msg_payload[ACK_RESET__STATUS] = ( ( reset_done == true ) ? 0 : 1 );
            prepare_ack( cmd_id, ORDER_ID__ACK_RESET, ACK_RESET_SIZE );

            if( reset_done == true )
            {
                vTaskDelay( 200 );    // Wait a few ms to send the message over usb
                NVIC_SystemReset( );  // and reset (even in debug mode)
            }
        }
        else
        {
            prepare_ack( cmd_id, ORDER_ID__CMD_ERROR, CMD_ERROR_SIZE );
        }
        break;

    case ORDER_ID__REQ_WRITE_GPIO:
        if( payload_len == REQ_WRITE_GPIO_SIZE )
        {
            bool success = decode_req_write_gpio( cmd_id, payload );

            tx_msg_payload[ACK_GPIO_WRITE__STATUS] = ( ( success == true ) ? 0 : 1 );
            prepare_ack( cmd_id, ORDER_ID__ACK_WRITE_GPIO, ACK_GPIO_WRITE_SIZE );
        }
        else
        {
            prepare_ack( cmd_id, ORDER_ID__CMD_ERROR, CMD_ERROR_SIZE );
        }
        break;

    case ORDER_ID__REQ_MULTIPLE_SPI:
    {
        uint16_t ret_len = device__spi_start_multiple( payload, tx_msg_payload, payload_len );
        prepare_ack( cmd_id, ORDER_ID__ACK_MULTIPLE_SPI, ret_len );
        break;
    }

    default:
        // Unknow command
        // dbg_printf_error("Command 0x%08X with id %d is unknow", order_id, cmd_id);
        prepare_ack( cmd_id, ORDER_ID__CMD_ERROR, CMD_ERROR_SIZE );
        break;
    }
}



/**
 * @brief Build ack frame
 *
 * @param cmd_id command id
 * @param order_id order id
 * @param buffer_len len of real payload (not the 4 first bytes)
 */
static void prepare_ack( const uint8_t cmd_id, const order_id_t order_id, const uint16_t buffer_len )
{
    tx_msg_buffer[CMD_OFFSET__ID]       = cmd_id;
    tx_msg_buffer[CMD_OFFSET__SIZE_MSB] = ( uint8_t )( buffer_len >> 8 );
    tx_msg_buffer[CMD_OFFSET__SIZE_LSB] = ( uint8_t ) buffer_len;
    tx_msg_buffer[CMD_OFFSET__CMD]      = ( uint8_t ) order_id;

    com_tx__post_message( tx_msg_buffer, buffer_len + CMD_OFFSET__DATA );
}


/**
 * @brief Enqueue buffer to be send on USB to host
 *
 * @param buffer buffer to send
 * @param buffer_len buffer length
 * @return true buffer have been send to the tx task and will be process
 * @return false fail to copy buffer
 */
void com_tx__post_message( const uint8_t* buffer, const uint16_t buffer_len )
{
	if ( xSemaphoreTake( sema_com_tx, pdMS_TO_TICKS( 10 ) ) != pdPASS )
    {
        Error_Handler( );
    }

    if( xMessageBufferSend( g_msgbuffer_com_tx, buffer, buffer_len, pdMS_TO_TICKS( 10 ) ) != buffer_len ) //This will send the message to the other task
    {
        Error_Handler( );
    }

    if( xSemaphoreGive( sema_com_tx ) != pdPASS )
    {
        Error_Handler( );
    }
}


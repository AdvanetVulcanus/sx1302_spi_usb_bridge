

#include <stdio.h>
#include "socket.h"
#include "wizchip_conf.h" 
#include "ethernet_poc.h"

/**
 * @brief Creates a socket struct
 *
 */

socket_str_t* socket_factory(uint8_t socket_number, uint8_t* bridgeip, uint16_t port)
{
    socket_str_t *socket_obj;
    socket_obj = (socket_str_t *)malloc(sizeof(socket_obj));
    socket_obj->buffer = (uint8_t *)malloc(sizeof(uint8_t));
    socket_obj->socket_number = socket_number;
    socket_obj->bridge_port = port;
    socket_obj ->ip[0] = bridgeip[0];
    socket_obj ->ip[1] = bridgeip[1];
    socket_obj ->ip[2] = bridgeip[2];
    socket_obj ->ip[3] = bridgeip[3];

    return socket_obj;
}

/**
 * @brief Frees a socket struct and close connection before free
 *
 */

void socket_destructor(socket_str_t *socket_obj)
{

    if (getSn_SR(socket_obj->socket_number) == SOCK_ESTABLISHED)
    {
        close(socket_obj->socket_number);
    };

    free(socket_obj);
}

/**
 * @brief Initializes socket
 *
 */

uint8_t ETHERNET_SOCKET_Init(socket_str_t *socket_obj)
{
    return socket(socket_obj->socket_number, Sn_MR_TCP, socket_obj->bridge_port, 0x0); // Creating socket
}

/**
 * @brief Establises socket connection
 *
 */
int32_t establish_connection_with_bridge(socket_str_t *socket_obj) // This function connects to the bridge
{

    int32_t ret; // return value for SOCK_ERRORs
    uint16_t size = 0, sentsize = 0;
    static uint16_t any_port = 50000;

    switch (getSn_SR(socket_obj->socket_number))
    {
    case SOCK_ESTABLISHED:                       // Successful connection
        if (getSn_IR(socket_obj->socket_number) & Sn_IR_CON) // Socket n interrupt register mask; TCP CON interrupt = connection with peer is successful
        {
            printf("%d:Connected to - %d.%d.%d.%d : %d\r\n", socket_obj->socket_number, socket_obj->bridge_ip[0], socket_obj->bridge_ip[1], socket_obj->bridge_ip[2], socket_obj->bridge_ip[3], socket_obj->bridge_port); // Information line

            setSn_IR(socket_obj->socket_number, Sn_IR_CON); // this interrupt should be write the bit cleared to '1'
        }

        break;

    case SOCK_CLOSE_WAIT: // closing sockets

        printf("%d:CloseWait\r\n", socket_obj->socket_number);
        if ((ret = disconnect(socket_obj->socket_number)) != SOCK_OK)
            return ret;
        printf("%d:Socket Closed\r\n", socket_obj->socket_number);
        break;

    case SOCK_INIT: // Trying to connect to socket

        printf("%d:Try to connect to the %d.%d.%d.%d : %d\r\n", socket_obj->socket_number, socket_obj->bridge_ip[0], socket_obj->bridge_ip[1], socket_obj->bridge_ip[2], socket_obj->bridge_ip[3], socket_obj->bridge_port);

        if ((ret = connect(socket_obj->socket_number, socket_obj->bridge_ip, socket_obj->bridge_port)) != SOCK_OK)
            return ret; //	Try to TCP connect to the TCP server (destination)
        break;

    case SOCK_CLOSED: // Closed connection
        close(socket_obj->socket_number);
        if ((ret = socket(socket_obj->socket_number, Sn_MR_TCP, any_port++, 0x00)) != socket_obj->socket_number)
        {
            if (any_port == 0xffff)
                any_port = 50000;
            return ret; // TCP socket open with 'any_port' port number
        }

        break;
    default:
        break;
    }
    return 1;
}

/**
 * @brief Receives from the bridge
 *  Receives bytes from the ETHERNET connection throught the socket
 */

void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
    uint16_t ptr = 0;
    uint32_t addrsel = 0;

    if (len == 0)
        return;
    ptr = getSn_TX_WR(sn);
    // M20140501 : implict type casting -> explict type casting
    // addrsel = (ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
    addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
    //
    WIZCHIP_WRITE_BUF(addrsel, wizdata, len);

    ptr += len;
    setSn_TX_WR(sn, ptr);
}
int32_t recieve_from_bridge(socket_str_t *socket_obj) // This function sends data to the bridge
{
    int32_t ret; // return value for SOCK_ERRORs
    uint16_t size = 0, sentsize = 0;

    static uint16_t any_port = 50000;

    switch (getSn_SR(socket_obj->socket_number))
    {
    case SOCK_ESTABLISHED:
        if (getSn_IR(socket_obj->socket_number) & Sn_IR_CON) // Socket n interrupt register mask; TCP CON interrupt = connection with peer is successful
        {

            printf("%d:Connected to - %d.%d.%d.%d : %d\r\n", socket_obj->socket_number, socket_obj->bridge_ip[0], socket_obj->bridge_ip[1], socket_obj->bridge_ip[2], socket_obj->bridge_ip[3], socket_obj->bridge_port);

            setSn_IR(socket_obj->socket_number, Sn_IR_CON); // this interrupt should be write the bit cleared to '1'
        }

        if ((size = getSn_RX_RSR(socket_obj->socket_number)) > 0) // Sn_RX_RSR: Socket n Received Size Register, Receiving data length
        {
            if (size > DATA_BUF_SIZE)
                size = DATA_BUF_SIZE;             // DATA_BUF_SIZE means user defined buffer size (array)
            ret = recv(socket_obj->socket_number, socket_obj->buffer, size); // Data Receive process (H/W Rx socket buffer -> User's buffer)

            if (ret <= 0)
                return ret; // If the received data length <= 0, receive failed and process end
            size = (uint16_t)ret;
            return size;
        }

        break;

    case SOCK_CLOSE_WAIT:

        // printf("%d:CloseWait\r\n",socket_obj->socket_number);

        if ((ret = disconnect(socket_obj->socket_number)) != SOCK_OK)
            return ret;

        printf("%d:Socket Closed\r\n", socket_obj->socket_number);

        break;

    case SOCK_INIT:

        printf("%d:Try to connect to the %d.%d.%d.%d : %d\r\n", socket_obj->socket_number, socket_obj->bridge_ip[0], socket_obj->bridge_ip[1], socket_obj->bridge_ip[2], socket_obj->bridge_ip[3], socket_obj->bridge_port);

        if ((ret = connect(socket_obj->socket_number, socket_obj->bridge_ip, socket_obj->bridge_port)) != SOCK_OK)
            return ret; //	Try to TCP connect to the TCP server (destination)
        break;

    case SOCK_CLOSED:
        close(socket_obj->socket_number);
        if ((ret = socket(socket_obj->socket_number, Sn_MR_TCP, any_port++, 0x00)) != socket_obj->socket_number)
        {
            if (any_port == 0xffff)
                any_port = 50000;
            return ret; // TCP socket open with 'any_port' port number
        }

        break;
    default:
        break;
    }
    return 1;
}

/**
 * @brief sends to the bridge
 *  sends bytes from the SPI and send to the bridge with the socket
 */
int32_t send_to_bridge(socket_str_t *socket_obj) // this function receives data from the bridge
{
    int32_t ret; // return value for SOCK_ERRORs
    uint16_t size = 0, sentsize = 0;

    static uint16_t any_port = 50000;

    switch (getSn_SR(socket_obj->socket_number))
    {
    case SOCK_ESTABLISHED:
        if (getSn_IR(socket_obj->socket_number) & Sn_IR_CON) // Socket n interrupt register mask; TCP CON interrupt = connection with peer is successful
        {

            printf("%d:Connected to - %d.%d.%d.%d : %d\r\n", socket_obj->socket_number,  socket_obj->socket_number[0], socket_obj->bridge_ip[1], socket_obj->bridge_ip[2], socket_obj->bridge_ip[3], socket_obj->bridge_port);

            setSn_IR(socket_obj->socket_number, Sn_IR_CON); // this interrupt should be write the bit cleared to '1'
        }

        while (size != sentsize)
        {
            ret = send(socket_obj->socket_number, socket_obj->buffer + sentsize, size - sentsize); // Data send process (User's buffer -> Destination through H/W Tx socket buffer)
            if (ret < 0)                                                // Send Error occurred (sent data length < 0)
            {
                close(socket_obj->socket_number); // socket close
                return ret;
            }
            sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
        }

        break;

    case SOCK_CLOSE_WAIT:

        if ((ret = disconnect(socket_obj->socket_number)) != SOCK_OK)
            return ret;

        printf("%d:Socket Closed\r\n", socket_obj->socket_number);

        break;

    case SOCK_INIT:

        printf("%d:Try to connect to the %d.%d.%d.%d : %d\r\n", socket_obj->socket_number, socket_obj->socket_number[0], socket_obj->socket_number[1], socket_obj->socket_number[2], socket_obj->socket_number[3], socket_obj->bridge_port);

        if ((ret = connect(socket_obj->socket_number, socket_obj->bridge_ip, socket_obj->bridge_port)) != SOCK_OK)
            return ret; //	Try to TCP connect to the TCP server (destination)
        break;

    case SOCK_CLOSED:
        close(socket_obj->socket_number);
        if ((ret = socket(socket_obj->socket_number, Sn_MR_TCP, any_port++, 0x00)) != socket_obj->socket_number)
        {
            if (any_port == 0xffff)
                any_port = 50000;
            return ret; // TCP socket open with 'any_port' port number
        }

        break;
    default:
        break;
    }
    return 1;
}



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


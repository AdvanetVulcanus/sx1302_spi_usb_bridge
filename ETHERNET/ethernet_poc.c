

#include <stdio.h>
#include "socket.h"
#include "wizchip_conf.h" 
#include "ethernet_poc.h"

/**
 * @brief Creates a socket struct
 *
 */

socket_str_t* socket_factory(uint8_t socket_number, uint8_t buf, uint16_t port)
{
    socket_str_t *socket_obj;
    socket_obj = (socket_str_t *)malloc(sizeof(socket_obj));

    socket_obj->buffer = (uint8_t *)malloc(sizeof(uint8_t));
    socket_obj->socket_number = socket_number;
    socket_obj->bridge_port = port;

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

void ETHERNET_SOCKET_Init(socket_str_t *socket_obj)
{
    socket(socket_obj->socket_number, Sn_MR_TCP, socket_obj->bridge_port, 0x0); // Creating socket
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
            sentsize = 0;
            wiz_send_data(socket_obj->socket_number, socket_obj->buffer, size);
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

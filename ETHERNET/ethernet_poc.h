
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define BRIDGE_SOCK 1
#define DATA_BUF_SIZE 2048


typedef struct socket_str
    {       
    uint8_t *buffer; /* Data stream */
    uint8_t socket_number;
    uint8_t bridge_ip[4];
    uint16_t bridge_port;
} socket_str_t;


void socket_destructor(socket_str_t *socket_obj);
void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint16_t len);


uint8_t ETHERNET_SOCKET_Init(socket_str_t *socket_obj);
int32_t establish_connection_with_bridge(socket_str_t *socket_obj);
int32_t recieve_from_bridge(socket_str_t *socket_obj);
int32_t send_to_bridge(socket_str_t *socket_obj);
socket_str_t* socket_factory(uint8_t socket_number, uint8_t* bridgeip, uint16_t port);


#ifdef __cplusplus
}
#endif

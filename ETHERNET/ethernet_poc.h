
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

void ETHERNET_SOCKET_Init(void)
int32_t send_to_bridge(uint8_t *buf)
int32_t receive_from_bridge(uint8_t *buf)
establish_connection_with_bridge()

#ifdef __cplusplus
}
#endif

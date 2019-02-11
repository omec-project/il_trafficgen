#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
typedef struct udp_sock_t {
	struct sockaddr_in my_addr;
	struct sockaddr_in other_addr;
	int sock_fd;
} udp_sock_t;

void iface_module_constructor(void);
int udp_init_socket(void);
int create_udp_socket(struct in_addr send_ip, struct in_addr recv_ip,
		uint16_t send_port, uint16_t recv_port, udp_sock_t *sock);
int udp_send_socket(void *data, int size);
int udp_recv_socket(void *data, int size);

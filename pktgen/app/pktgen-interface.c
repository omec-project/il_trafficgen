#include "pktgen-interface.h"
#include "pktgen-tool.h"
udp_sock_t my_sock;
void iface_module_constructor(void) {
	/* Register generator socket details */
	udp_init_socket();
}

/**
 * Init sockets for send ad recv.
 *
 * @return
 *  0 - success
 *  -1 - fail
 */
int udp_init_socket(void)
{
	if (traffic_gen_as == IL_TRAFFIC_GEN) {
		/* create a UDP socket for generator */
    	if (create_udp_socket(resp_host_ip, gen_host_ip, resp_host_port,
							  gen_host_port, &my_sock) < 0) {
        	rte_exit(EXIT_FAILURE, "Create Generator UDP Socket Failed "
            	"for IP %s:%u!!!\n", inet_ntoa(gen_host_ip), gen_host_port);
		}
	} else {
		/* create a UDP socket for responder */
    	if (create_udp_socket(gen_host_ip, resp_host_ip, gen_host_port,
						      resp_host_port, &my_sock) < 0) {
        	rte_exit(EXIT_FAILURE, "Create Responder UDP Socket Failed "
            	"for IP %s:%u!!!\n", inet_ntoa(resp_host_ip), resp_host_port);
		}
	}
    return 0;
}

/**
 * @brief API to create udp socket.
 * @return
 *  0 - success
 *  -1 - fail
 */
int create_udp_socket(struct in_addr send_ip, struct in_addr recv_ip,
		uint16_t send_port, uint16_t recv_port, udp_sock_t *sock)
{
    sock->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock->sock_fd == -1) {
        perror("socket error: ");
        close(sock->sock_fd);
        return -1;
    }
    memset(&sock->other_addr, 0x0, sizeof(struct sockaddr_in));

    sock->other_addr.sin_family = AF_INET;
    sock->other_addr.sin_port = htons(send_port);
    sock->other_addr.sin_addr = send_ip;

    memset(&sock->my_addr, 0x0, sizeof(struct sockaddr_in));
    sock->my_addr.sin_family = AF_INET;
    sock->my_addr.sin_port = htons(recv_port);
    sock->my_addr.sin_addr = recv_ip;
   	if (bind(sock->sock_fd, (struct sockaddr *)&sock->my_addr,
           sizeof(struct sockaddr_in)) == -1)
        return -1;
    return 0;
}

/**
 * @brief API to send udp pakcets
 * @return
 *  0 - success
 *  -1 - fail
 */
int udp_send_socket(void *data, int size) {
	int res = sendto(my_sock.sock_fd, data, size, MSG_DONTWAIT,
					(struct sockaddr *)&my_sock.other_addr,
					sizeof(struct sockaddr_in));
	if ( res < 0) { 
        perror("Failed to send msg !!!:");
		return -1;
	}
    return 0;
}

/**
 * @brief API to recv udp pakcets
 * @return
 *  0 - success
 *  -1 - fail
 */
int udp_recv_socket(void *data, int size) {
	int n, rv;
    int bytes_recvd = 0;
	fd_set readfds;
	struct timeval tv;
	FD_ZERO(&readfds);
	FD_SET(my_sock.sock_fd, &readfds);
	n = my_sock.sock_fd + 1;
	tv.tv_sec = 0;
	tv.tv_usec = 100;
	rv = select(n, &readfds, NULL, NULL, &tv);
	if (rv == -1)   {
		perror("select");
	} else if (rv > 0) {
		if (FD_ISSET(my_sock.sock_fd, &readfds)) {	
    		bytes_recvd = recvfrom(my_sock.sock_fd, data, size,
						MSG_DONTWAIT, NULL, NULL);
    		if (errno != EAGAIN && bytes_recvd < size) {
        		perror("Failed to recv msg !!!\n");
        		return -1;
    		}
		}
	}
    return bytes_recvd;
}

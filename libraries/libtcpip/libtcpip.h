#ifndef _LIBTCPIP_H_
#define _LIBTCPIP_H_
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32")

#ifdef __cplusplus
extern "C" {
#endif

#define UDP_MSG_MAX_LEN		1460	//MTU
#define TCP_MSG_MAX_LEN		1452

	int init_socket();
	int close_socket();
	int shutdown_socket();

	int tcp_socket(SOCKET* s);
	int tcp_bind(SOCKET* s, u_short port);
	int tcp_listen(SOCKET* s);
	int tcp_accept(SOCKET* s, struct sockaddr_in* preomte_addr, SOCKET* rs);
	int tcp_connect(SOCKET* s, u_long remote, u_short port);
	int tcp_send(SOCKET* s, void* buf);
	int tcp_recv(SOCKET* s, void* buf);

	int udp_socket(SOCKET* s);
	int udp_sendto(SOCKET* s, void* buf, u_long remote, u_short port);
	int udp_recvfrom(SOCKET* s, char* buf, char* ip, u_short port);

	int get_addrinfo();

typedef void (*cb_msg_callback) (SOCKET* s, int sendrecv);

	int set_cb_msg(cb_msg_callback cbmsg);
#define udp_bind(s, port) tcp_bind(s, port)


#ifdef __cplusplus
}
#endif

#endif
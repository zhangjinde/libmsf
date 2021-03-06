/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include <netinet/tcp.h>
#include <sys/uio.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#endif

#include <msf_utils.h>

#define invalid_socket      -1

#define max_epoll_event     256


#define	PREFIX_UNIX_PATH    "/var/" /* +5 for pid = 14 chars */

#define MSF_RPC_UNIX_SERVER "/var/msf_rpc_srv.sock"
#define MSF_RPC_UNIX_DLNA   "/var/msf_rpc_dlna.sock"
#define MSF_RPC_UNIX_UPNP   "/var/msf_rpc_upnp.sock"

#define local_host_v4       "127.0.0.1"
#define local_host_v6       "[::1]"
#define local_port          "9999"

#define RESOLV_FILE         "/etc/resolv.conf"
#define ROUTE_FILE          "/proc/net/route"
#define IF_INET6_FILE       "/proc/net/if_inet6"
#define IPV6_ROUTE_FILE     "/proc/net/ipv6_route"
#define DOMAIN_ADDR_LEN     64 

/* IP地址合法性判断宏定义,只针对IPv4地址 */
#define NET_SWAP_32(x)                      ((((x)&0xff000000) >> 24)\
                                            | (((x)&0x00ff0000) >> 8)\
                                            | (((x)&0x0000ff00) << 8)\
                                            | (((x)&0x000000ff) << 24))
#define IN_IS_ADDR_NETMASK(mask)            (!((~(NET_SWAP_32(mask)) + 1) & (~(NET_SWAP_32(mask)))))
#define IN_IS_ADDR_LOOPBACK(ip)             (((ip)&0xff) == 0x7F)   //127
#define IN_IS_ADDR_MULTICAST(ip)            (((ip)&0xf0) == 0xE0)   //224~239
#define IN_IS_ADDR_RESERVE_MULTICAST(ip)    ((((ip)&0xff) == 0xE0)\
                                            && ((((ip)&0x00ff0000) >> 16) == 0)\
                                            && ((((ip)&0x0000ff00) >> 8) == 0)\
                                            && ((((ip)&0xff000000) >> 24) <= 0x12))//保留的多播地址，224.0.0.0~224.0.0.18
#define IN_IS_ADDR_BROADCAST(ip, mask)      (((ip)&(~(mask))) == (~(mask)))	//广播地址，即主机号全1
#define IN_IS_ADDR_NETADDR(ip, mask)        (!((ip)&(~(mask))))	//网络地址，即主机号全零
#define IN_IS_ADDR_UNSPECIFIED(ip)          (!(ip)) //地址为全零



#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN   (sizeof("255.255.255.255") - 1)
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN                                                 \
    (sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255") - 1)
#endif

#ifndef UNIX_ADDRSTRLEN
#define UNIX_ADDRSTRLEN                                                  \
    (sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))
#endif

#ifndef ADDRSTRLEN
#define ADDRSTRLEN          (INET6_ADDRSTRLEN + 9)
#endif

#define SIN(sa)             ((struct sockaddr_in *)sa)
#define SIN6(sa)            ((struct sockaddr_in6 *)sa)
#define SINU(sa)            ((struct sockaddr_un *)sa)
#define PADDR(a)            ((struct sockaddr *)a)

enum net_protocol {
    TCPV4	= IPPROTO_TCP, /**< TCP protocol */
    TCPV6,
    UDPV4	= IPPROTO_UDP, /**< UDP protocol */
    UDPV6,
    UNIX, 	/* Unix sockets*/
    MUTICAST,
};

#define IS_TCP(x) 		(x == SOCK_STREAM || x == SOCK_STREAM)
#define IS_UDP(x) 		(x == SOCK_DGRAM || x == SOCK_DGRAM)
#define IS_UNIX(x) 		(x == SOCK_STREAM)
#define IS_MUTICATS(x) 	(x == SOCK_STREAM)

/* Macro: True iff the IPv4 address 'addr', in host order, is in 127.0.0.0/8
 */
#define MSF_V4ADDR_IS_LOCALHOST(addr) (((addr)>>24) == 127)

/* Macro: True iff the IPv4 address 'addr', in host order, is a class D
 * (multiclass) address.
 */
#define MSF_V4ADDR_IS_CLASSD(addr) ((((addr)>>24) & 0xf0) == 0xe0)

/* macro should be declared in netinet/in.h even in POSIX compilation
 * but it appeared that it is not defined on some BSD system
 */
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41	
#endif

/* MinGW does not define IPV6_V6ONLY */
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

struct denied_address {
    s32     family;     /**< AF family (AF_INET or AF_INET6) */
    s8      addr[16];   /**< IPv4 or IPv6 address */
    s8      mask[16];   /**< Network mask of the address */
    s32     port;       /**< Network port of the address  */
    struct denied_address* next;    /**< For list management */
} __attribute__((__packed__));

typedef struct in6_ifreq
{
    struct in6_addr ifr6_addr;
    unsigned int ifr6_prefixlen;
    unsigned int ifr6_ifindex;
}IPV6_REQ;

/* A multi-family sockaddr. */
typedef union {
    struct sockaddr_in sa_v4;
    struct sockaddr_in6 sa_v6;

    struct sockaddr_un sa_un;

    //struct sockaddr sa; 
    //struct sockaddr_storage sa_stor; 
} sock_addr_base;

s32 isipaddr(const s8 *ip, u32 af_type);

s32 socket_ip_str(s32 fd, s8 **buf,  u32 size, u32 *len);

s32 socket_cork_flag(s32 fd, u32 state);
s32 socket_blocking(s32 fd);
s32 socket_nonblocking(s32 fd);

s32 socket_alive(s32 fd);
s32 socket_linger(s32 fd);
s32 socket_timeout(s32 fd, u32 us);
s32 socket_reuseaddr(s32 fd);
s32 socket_reuseport(s32 fd);
s32 socket_closeonexec(s32 fd);

s32 socket_tcp_defer_accept(s32 fd);
s32 socket_tcp_nodelay(s32 fd);
s32 socket_tcp_fastopen(s32 fd);


s32 socket_create(u32 domain, u32 type, u32 protocol);
s32 socket_bind(s32 fd, const struct sockaddr *addr,
                socklen_t addrlen, u32 backlog);

s32 recvn(s32 fd, void* const buf_, u32 n);
s32 sendn(const s32 fd, const void * const buf_, 
            u32 count, const u32 timeout);

s32 readn(s32 fd, void* const buf_, u32 n);
s32 writen(const s32 fd, const void * const buf_, 
                u32 count, const u32 timeout);

s32 udp_recvn(s32 fd, void* const buf_, u32 n);
s32 udp_sendn(const s32 fd, const void * const buf_, 
                u32 count, const u32 timeout);

s32 msf_sendmsg(s32 clifd, struct msghdr *msg);

s32 msf_recvmsg(s32 clifd, struct msghdr *msg);


void drain_fd(s32 fd, u32 n_packets);

void socket_debug(s32 fd);

s32 server_unix_socket(s32 backlog, s8 *unixpath, s32 access_mask);
s32 server_socket(s8 *interf, s32 protocol, s32 port, s32 backlog);

s32 connect_to_unix_socket(const s8 *cname, const s8 *sname);
s32 connect_to_host (const s8 *host, const s8 *port);

void socket_maximize_sndbuf(const s32 sfd);


#define MACADDR_IS_ZERO(x) \
  ((x[0] == 0x00) && \
   (x[1] == 0x00) && \
   (x[2] == 0x00) && \
   (x[3] == 0x00) && \
   (x[4] == 0x00) && \
   (x[5] == 0x00))

s32 getsyshwaddr(s8 *buf, s32 len);
s32 get_remote_mac(struct in_addr ip_addr, u8 *mac);
void reload_ifaces(s32 notify);

s32 get_ipaddr(s8 *iface, s8 *ip, s32 len);
s32 get_ifaddr(s8 *iface, s8 *ip, s32 len);


s32 OpenAndConfMonitorSocket();
void ProcessMonitorEvent(s32 s);

s32 msf_eventfd(u32 initval, s32 flags);
s32 msf_eventfd_notify(s32 efd);
s32 msf_eventfd_clear(s32 efd); 

s32 msf_epoll_create(void);
s32 msf_add_event(s32 epfd, s32 clifd, short event, void *p);
s32 msf_mod_event(s32 epfd, s32 clifd, short event);
s32 msf_del_event(s32 epfd, s32 clifd);
#endif

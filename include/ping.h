#ifndef PING
#define PING

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/ip.h>
#include <linux/icmp.h>

#include <linkedList.h>

 


struct target{
  in_addr_t addr;
};

struct ping_th_args{
  struct list *queue;
  in_addr_t src;
  int sockfd;
};

char *create_packet();
void set_src(char *packet, in_addr_t src);
void set_dst(char *packet, in_addr_t dst);
int create_raw_socket();
int send_ping(char* packet, in_addr_t dst, int sockfd);
in_addr_t get_ip(char *iface, size_t ifname_len, int sockfd);
void *recv_loop(void *sockfd);
void *ping_loop(void *argsin);



#endif

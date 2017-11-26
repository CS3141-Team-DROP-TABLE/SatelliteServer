#ifndef CLIENT
#define CLIENT

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/ip.h>
#include <linux/icmp.h>

#include <linkedList.h>
#include <serverConnect.h>
#include <AVL.h>
#include <stringMap.h>
#include <configLoader.h>

#include <sys/time.h>

struct target_profile{
  in_addr_t adr;
  struct timeval lastping;
  char unreachable;
  char timeout;
  int pings_out;
};

struct ping_profile{
  in_addr_t dst;
  u_int16_t id;
  struct timeval sent;
};


struct ping_th_args{
  struct list *queue;
  struct tree *sent_pings;
  struct tree *targets;
  in_addr_t src;
  int sockfd;
  int *run;
};

struct ping_recv_th_args{
  struct connection *conn;
  struct tree *targets;
  struct list *ping_queue;
  struct tree *sent_pings;
  int sockfd;
  int *run;
};

struct server_th_args{
  struct tree *targets;
  struct list *ping_queue;
  struct connection *conn;
  int *run;
};

struct queue_th_args{
  struct list *queue;
  struct tree *sent_pings;
  struct tree *targets;
  in_addr_t src;
  int sockfd;
  int *run;
};


#endif

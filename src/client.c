
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <client.h>
#include <ping.h>
#include <linkedList.h>
#include <serverConnect.h>


#define CAFILE "intermediate.cert.pem"
#define SERVERNAME "nhs_server_domain.com"


void *ping_loop(void *argsin){
  struct list *queue = ((struct ping_th_args*)argsin)->queue;
  int sockfd = ((struct ping_th_args*)argsin)->sockfd;

  char *pkt = create_packet();
  set_src(pkt, ((struct ping_th_args*)argsin)->src);

  set_sock_opt(sockfd);
  
  struct target *t;
  while(1){
    
    t = (struct target*)ll_pq_dequeue(queue);
    if(t == NULL){
      usleep(750);
    }else if(t->addr == 0){
      free(t);
      break;
    }else{
      printf("ping sent\n");
      struct iphdr *ip = (struct iphdr*)pkt;
      struct icmphdr *icmp = (struct icmphdr*)(pkt+sizeof(struct iphdr));
      printf("ICMP: %d, Checksum: %04x\n", icmp->type, icmp->checksum);
      send_ping(pkt, t->addr, sockfd);
      free(t);
    }
  }

}



void *recv_loop(void *sockfd){
  char buffer[1024];
  int ret;
  struct iphdr *ip = (struct iphdr*)buffer;
  struct sockaddr_in rec_str;
  while(1){
    ret = recv(*(int*)sockfd, buffer, 1024, 0);
    if(ret >= 0){
      rec_str.sin_addr.s_addr = ip->saddr;
      printf("ping received of size %d from %s\n\n", ret, inet_ntoa(rec_str.sin_addr));
    }
    usleep(75);

  }

}




int main(){

  
  


  struct list ping_queue;
  ll_initialize(&ping_queue);


  int sockfd = create_raw_socket();
  in_addr_t src = get_ip("wlp9s0\0\0", 6, sockfd);
  
  struct ping_th_args snd_args;
  snd_args.queue = &ping_queue;
  snd_args.src = src;
  snd_args.sockfd = sockfd;
  
  pthread_t th_recv, th_snd;
  void **retval;
  pthread_create(&th_recv, NULL, recv_loop, (void*)&sockfd);
  pthread_create(&th_snd, NULL, ping_loop, (void*) &snd_args);


  struct in_addr dst;
  inet_aton("8.8.8.8", &dst);
  
  struct target tr;
  struct target *t;

  tr.addr = dst.s_addr;

  /*/

  const char *server = "127.0.0.1\0";

  struct connection conn;

  conn.port = 5556;
  conn.server = (char*)malloc(sizeof(char)*strlen(server));
  memcpy(conn.server, server, strlen(server));
 
  open_tcp(&conn);

  connect_TLS(&conn, CAFILE, SERVERNAME, strlen(SERVERNAME));

  tls_send(&conn, "hello\n\0\0", 7);
  
  char buff[2048];
  tls_recv(&conn, buff, 2047);
  printf("%s\n", buff);
  //*/
  
  for(int i = 0; i < 100; i++){ 
    t = (struct target*)malloc(sizeof(struct target));
    memcpy(t, &tr, sizeof(struct target));
    if(t == NULL){
      printf("ERROR\n\n");
    }
    ll_pq_enqueue(&ping_queue, t, 1);
    sleep(1);
  }
  
  pthread_join(th_recv, retval);
  pthread_join(th_snd, retval);
  close(sockfd);

}

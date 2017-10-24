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

#include <ping.h>
#include <linkedList.h>

int main(){

  
  
  int sockfd = create_raw_socket();
  in_addr_t src = get_ip("wlp9s0\0\0", 6, sockfd);
  
  
  


  struct list ping_queue;
  ll_initialize(&ping_queue);
  
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

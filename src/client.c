
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
#include <errno.h>
#include <time.h>

#include <sys/time.h>

#include <client.h>
#include <ping.h>
#include <linkedList.h>
#include <serverConnect.h>
#include <AVL.h>
#include <stringMap.h>
#include <configLoader.h>



#define CAFILE "intermediate.cert.pem"
#define SERVERNAME "nhs_server_domain.com"

int compare(void *a, void *b, size_t sz){
  return memcmp(a, b, sz);
}

u_int16_t get_unique(){
  static u_int16_t id = 0;
  return id++;
}


void *ping_loop(void *argsin){ 
  struct ping_th_args *args = (struct ping_th_args*)argsin;

  set_sock_opt(args->sockfd);
  
  char *packet = create_packet();
  set_src(packet, args->src);

  struct ping_profile *profile = NULL;
  in_addr_t *dst;
  u_int16_t id;
  while(args->run){
    dst = (in_addr_t*)ll_pq_dequeue(args->queue);
    if(dst != NULL){
      set_dst(packet, *dst);
      id = get_unique();
      set_id(packet, id);
      profile = malloc(sizeof(struct ping_profile));
      profile->dst = *dst;
      profile->id = id;
      avl_insert(args->sent_pings, &profile->id, profile, compare, sizeof(u_int16_t));
      gettimeofday(&profile->sent, NULL);

      int ret = send_ping(packet, *dst, args->sockfd);
      
      struct in_addr ia;
      ia.s_addr = *dst;
      printf("sent: %s at %lu.%6lu\t ret:%d\n", inet_ntoa(ia), profile->sent.tv_sec, profile->sent.tv_usec, ret);

      
      free(dst);
    } else {
      sleep(1);
    }
  }
}



void *recv_loop(void *argsin){
  /*
  struct ping_recv_th_args *args = (struct ping_recv_th_args*)argsin;

  int sockfd = args->sockfd;

  struct timespec tm;
  struct timespec *tm_sent;
  void *sent_ping;
  struct target *t;
  
  char buffer[1024];
  int ret;
  struct iphdr *ip = (struct iphdr*)buffer;
  struct sockaddr_in rec_str;
  struct in_addr inadr;

  set_sock_opt(sockfd);
  while(args->run){
    ret = recv(sockfd, buffer, 1024, 0);
    clock_gettime(CLOCK_MONOTONIC, &tm);
    
    if(ret < 0){
      printf("Recv error: %s\n", strerror(errno));
    }
    if(ret >= 0 && ip->protocol == IPPROTO_ICMP){
      rec_str.sin_addr.s_addr = ip->saddr;

      sent_ping = ll_search_node(args->sent_pings, &ip->saddr, sizeof(in_addr_t));
      if(sent_ping != NULL){

	tm_sent = sent_ping+sizeof(in_addr_t);	
	snprintf(buffer, 1023, "ping received of size %d from %s in %ldms\n\n", ret, inet_ntoa(rec_str.sin_addr), (tm.tv_nsec - tm_sent->tv_nsec)/1000000);
	free(sent_ping);

	t = malloc(sizeof(struct target));
	t->addr = rec_str.sin_addr.s_addr;
	ll_pq_enqueue(args->ping_queue, t, 0);
	t = NULL;
	
	printf("Sending to central server: %s\n", buffer);
	tls_send(args->conn, buffer, (strlen(buffer) < 1023)? strlen(buffer) : 1023);
	
	memset(buffer, 0, 1024);
      }
      

    }
    usleep(1);
  }
  */
    
}



void *server_loop(void *argsin){
  struct server_th_args *args = (struct server_th_args*)argsin;
  const int SIZE = 2048;
  char buff[SIZE];

  tls_send(args->conn, "hello\n\0\0", 7);
  
  
  printf("%s\n", buff);

  in_addr_t *target;
  struct target *t;
  struct sockaddr_in tg;

  struct target_profile *tp;
  
  while(*(args->run)){
    memset(buff, 0, SIZE);
    tls_recv(args->conn, buff, SIZE-1);

    if(strncmp(buff, "Host:", 5) == 0){
      printf("Recived Host! %s\n", buff);
      in_addr_t tgt = inet_addr(&buff[5]);
      if(avl_search(args->targets, &tgt, compare, sizeof(in_addr_t)) == NULL){
	tp = malloc(sizeof(struct target_profile));
	tp->pings_out = 0;
	gettimeofday(&tp->lastping, NULL);
	avl_insert(args->targets, &tgt, tp, compare, sizeof(in_addr_t));
	printf("Seconds: %lu\n", tp->lastping.tv_sec);
      }
    }


  }

}

void queue_viable(void *target, void *profile, void *queue){
  in_addr_t *tgt = (in_addr_t*)target;
  struct target_profile *tp = (struct target_profile*)profile;
  struct list *q = (struct list*)queue;

  struct timeval time;
  gettimeofday(&time, NULL);


  struct in_addr printable;
  if(tp->pings_out < 3 && (time.tv_sec - tp->lastping.tv_sec) > 30){
    printable.s_addr = *tgt;
    printf("Queueing ping for %s\n", inet_ntoa(printable));
    tp->pings_out++;
    gettimeofday(&tp->lastping, NULL);
    ll_pq_enqueue(q, tgt, 0);

  }
  

}

void *queue_loop(void *argsin){
  struct queue_th_args *args = (struct queue_th_args*)argsin;

  while(args->run){
    avl_apply_to_all(args->targets, queue_viable, args->queue);
    sleep(5);
  }
}

int main(){

  struct list ping_queue;
  ll_initialize(&ping_queue);
  struct tree targets;
  avl_init(&targets);
  struct tree sent_pings;
  avl_init(&sent_pings);
  
  int sockfd = create_raw_socket();
  in_addr_t src = get_ip("wlp9s0\0\0", 6, sockfd);
  

  const char *server = "127.0.0.1\0";
  struct connection conn;


  conn.port = 5556;
  conn.server = (char*)malloc(sizeof(char)*strlen(server));
  memcpy(conn.server, server, strlen(server));
 
  open_tcp(&conn);

  connect_TLS(&conn, CAFILE, SERVERNAME, strlen(SERVERNAME));


  int run = 1;

  struct ping_th_args snd_args;
  snd_args.queue = &ping_queue;
  snd_args.sent_pings = &sent_pings;
  snd_args.targets = &targets;
  snd_args.src = src;
  snd_args.sockfd = sockfd;
  snd_args.run = &run;

  struct ping_recv_th_args recv_args;
  recv_args.conn = &conn;
  recv_args.sent_pings = &sent_pings;
  recv_args.ping_queue = &ping_queue;
  recv_args.sockfd = sockfd;
  recv_args.run = &run;
  
  struct server_th_args srv_args;
  srv_args.targets = &targets;
  srv_args.ping_queue = &ping_queue;
  srv_args.conn = &conn;
  srv_args.run = &run;


  struct ping_th_args qur_args;
  qur_args.queue = &ping_queue;
  qur_args.sent_pings = &sent_pings;
  qur_args.targets = &targets;
  qur_args.src = src;
  qur_args.sockfd = sockfd;
  qur_args.run = &run;

  pthread_t th_recv, th_snd, th_qur, th_srv;
  void **retval;
  pthread_create(&th_recv, NULL, recv_loop, (void*)&recv_args);
  pthread_create(&th_snd, NULL, ping_loop, (void*) &snd_args);  
  pthread_create(&th_srv, NULL, server_loop, (void*)&srv_args);
  pthread_create(&th_qur, NULL, queue_loop, (void*)&qur_args);

  pthread_join(th_srv, retval);
  pthread_join(th_qur, retval);
  pthread_join(th_recv, retval);
  pthread_join(th_snd, retval);
  close(sockfd);

}

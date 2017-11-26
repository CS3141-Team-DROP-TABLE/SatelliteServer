
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
      
      free(dst);
    } else {
      sleep(1);
    }
  }
}



void *recv_loop(void *argsin){
  struct ping_recv_th_args *args = (struct ping_recv_th_args*)argsin;

  char buffer[1024];
  char sndbuffer[1024];
  int ret;
  struct iphdr *ip = (struct iphdr*)buffer;
  struct icmphdr *icmp = (struct icmphdr*)(buffer+sizeof(struct iphdr));

  struct timeval rec_time;
  
  struct in_addr inadr;
  struct target_profile *tp;
  
  set_sock_opt(args->sockfd);
  
  while(args->run){
    ret = recv(args->sockfd, buffer, 1024, 0);
    gettimeofday(&rec_time, NULL);
    
    if(ret < 0){
      printf("Recv error: %s\n", strerror(errno));
    }
    if(ret >= 0 && ip->protocol == IPPROTO_ICMP){
      struct in_addr printable;
      printable.s_addr = ip->saddr;
      
      u_int16_t id = icmp->un.echo.id;
      struct ping_profile *profile = avl_search(args->sent_pings, &id, compare, sizeof(u_int16_t));
      
      
      if(profile != NULL && memcmp(&profile->dst, &ip->saddr, sizeof(in_addr_t)) == 0){
	uint64_t time_sent = profile->sent.tv_sec*(uint64_t)1000000 + profile->sent.tv_usec;
	uint64_t time_recv = rec_time.tv_sec*(uint64_t)1000000 + rec_time.tv_usec;

	tp = avl_search(args->targets, &profile->dst, compare, sizeof(in_addr_t));
	
	snprintf(sndbuffer, 1024, "Recv:%s=%lu;\n", inet_ntoa(printable), (time_recv - time_sent)/1000);
	printf("%s",sndbuffer);
	tp->pings_out--;

	tls_send(args->conn, sndbuffer, (strlen(buffer) < 1023)? strlen(sndbuffer) : 1023);
	
      } else if(profile != NULL) {
         tp = avl_search(args->targets, &profile->dst, compare, sizeof(in_addr_t));
	if(tp){
	  printf("Unreach\n");
	  tp->unreachable = 1;
	  tp->pings_out--;
	}

      } else {
	printf("received undocumented reply\n");
      }
      
      //tls_send(args->conn, buffer, (strlen(buffer) < 1023)? strlen(buffer) : 1023);
	
	
      }
      
    memset(buffer, 0, 1024);
    usleep(1);
  }
    
}



void *server_loop(void *argsin){
  struct server_th_args *args = (struct server_th_args*)argsin;
  const int SIZE = 2048;
  char buff[SIZE];

  tls_send(args->conn, "hello\n\0\0", 7);

  in_addr_t *target;
  struct target *t;
  struct sockaddr_in tg;

  struct target_profile *tp;
  
  while(*(args->run)){
    memset(buff, 0, SIZE);
    tls_recv(args->conn, buff, SIZE-1);

    if(strncmp(buff, "Host:", 5) == 0){
      in_addr_t tgt = inet_addr(&buff[5]);
      if(avl_search(args->targets, &tgt, compare, sizeof(in_addr_t)) == NULL){
	tp = malloc(sizeof(struct target_profile));
	tp->adr = tgt;
	tp->unreachable = 0;
	tp->timeout = 0;
	tp->pings_out = 0;
	gettimeofday(&tp->lastping, NULL);
	avl_insert(args->targets, &tp->adr, tp, compare, sizeof(in_addr_t));
      }
    }

  }

}

void queue_viable(void *target, void *profile, void *queue){
  struct target_profile *tp = (struct target_profile*)profile;
  struct list *q = (struct list*)queue;

  struct timeval time;
  gettimeofday(&time, NULL);

  in_addr_t *tgt;
  struct in_addr printable;
  if(tp->pings_out < 3 && (time.tv_sec - tp->lastping.tv_sec) > ((tp->unreachable || tp->timeout)? 200 : 10)){
    tgt = malloc(sizeof(in_addr_t));
    memcpy(tgt, target, sizeof(in_addr_t));
    printable.s_addr = *tgt;
    tp->pings_out++;
    gettimeofday(&tp->lastping, NULL);
    ll_pq_enqueue(q, tgt, 0);

  }
  

}

void *queue_loop(void *argsin){
  struct queue_th_args *args = (struct queue_th_args*)argsin;

  while(args->run){
    avl_apply_to_all(args->targets, queue_viable, args->queue);
    sleep(2);
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
  recv_args.targets = &targets;
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

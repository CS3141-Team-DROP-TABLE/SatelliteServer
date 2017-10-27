#ifndef SERVER_CONNECT
#define SERVER_CONNECT
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


struct connection{
  int port;
  char *server;
  struct sockaddr_in sa;
  int sockfd;
  gnutls_certificate_credentials_t xcred;
  gnutl_session_t session;
};





#endif 

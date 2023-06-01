#ifndef CARROW_ADDR_H
#define CARROW_ADDR_H


#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 


int
sockaddr_parse(struct sockaddr *saddr, const char *addr, unsigned short port);


char *
sockaddr_dump(struct sockaddr *addr);


#endif

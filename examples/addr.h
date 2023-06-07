// copyright 2023 vahid mardani
/*
 * this file is part of carrow.
 *  carrow is free software: you can redistribute it and/or modify it under 
 *  the terms of the gnu general public license as published by the free 
 *  software foundation, either version 3 of the license, or (at your option) 
 *  any later version.
 *  
 *  carrow is distributed in the hope that it will be useful, but without any 
 *  warranty; without even the implied warranty of merchantability or fitness 
 *  for a particular purpose. see the gnu general public license for more 
 *  details.
 *  
 *  you should have received a copy of the gnu general public license along 
 *  with carrow. if not, see <https://www.gnu.org/licenses/>. 
 *  
 *  author: vahid mardani <vahid.mardani@gmail.com>
 */
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

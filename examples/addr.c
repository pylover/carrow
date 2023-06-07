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
#include "addr.h"

#include <stdio.h>
#include <string.h>


#define TMPMAX 256
static char addrtemp[TMPMAX];


char *
sockaddr_dump(struct sockaddr *addr) {
    struct sockaddr_in *addrin = (struct sockaddr_in*) addr;
    snprintf(addrtemp, TMPMAX, "%s:%d", inet_ntoa(addrin->sin_addr),
        ntohs(addrin->sin_port));
    return addrtemp;
}


int
sockaddr_parse(struct sockaddr *saddr, const char *addr,
        unsigned short port) {
    struct sockaddr_in *addrin = (struct sockaddr_in*)saddr;

    memset(addrin, 0, sizeof(struct sockaddr_in));
    addrin->sin_family = AF_INET;
    if (addr == NULL) {
        addrin->sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (inet_pton(AF_INET, addr, &addrin->sin_addr) <= 0) {
        return -1;
    }
    addrin->sin_port = htons(port);
    return 0;
}

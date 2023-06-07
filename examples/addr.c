// Copyright 2023 Vahid Mardani
/*
 * This file is part of Carrow.
 *  Carrow is free software: you can redistribute it and/or modify it under 
 *  the terms of the GNU General Public License as published by the Free 
 *  Software Foundation, either version 3 of the License, or (at your option) 
 *  any later version.
 *  
 *  Carrow is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along 
 *  with Carrow. If not, see <https://www.gnu.org/licenses/>. 
 *  
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
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

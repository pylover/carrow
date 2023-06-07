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
#include "tty.h"

#include <clog.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>


static bool dirty = false;
static struct termios ttysave;


static void
stdin_restore() {
    if (!dirty) {
        return;
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &ttysave)) {
        ERROR("Cannot restore tty attributes");
    }
}


static int
stdin_noncanonical() {
    struct termios ttystate;

    // preserve  the terminal state
    if (tcgetattr(STDIN_FILENO, &ttystate)) {
        ERROR("Cannot get tty attributes");
        return -1;
    }
    ttysave = ttystate;
    dirty = true;
    atexit(stdin_restore);
    // ttystate.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // ttystate.c_oflag &= ~(OPOST);
    // ttystate.c_cflag |= (CS8);
    // ttystate.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    ttystate.c_cc[VMIN] = 0;
    ttystate.c_cc[VTIME] = 1;

    // set the terminal attributes.
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &ttystate)) {
        ERROR("Cannot set tty attributes");
        return -1;
    }

    return 0;
}


static int
_nonblock(int fd) {
    if (!isatty(fd)) {
        ERROR("Pipes or regular files are not supported");
        errno = 0;
        return -1;
    }

    int status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    if (status == -1) {
        return -1;
    }

    return 0;
}


int
stdin_nonblock() {
    if (_nonblock(STDIN_FILENO)) {
        return -1;
    }

    return stdin_noncanonical();
}


int
stdout_nonblock() {
    return _nonblock(STDOUT_FILENO);
}

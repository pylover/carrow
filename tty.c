#include "tty.h"

#include <termios.h>


static struct termios ttysave;


void
stdnocannonical() {
    tcsetattr(STDIN_FILENO, TCSANOW, &ttysave);
}


void 
stdcannonical() {
    struct termios ttystate;

    // preserve  the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);
    ttysave = ttystate;

    ttystate.c_lflag &= ~(ICANON | ECHO);
    
    //minimum of number input read.
    ttystate.c_cc[VMIN] = 1;
    ttystate.c_cc[VTIME] = 0;

    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}



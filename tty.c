#include "tty.h"

#include <unistd.h>
#include <termios.h>
#include <stdbool.h>


static bool dirty = false;
static struct termios ttysave;


int
stdin_restore() {
    if (!dirty) {
        return 0;
    };
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &ttysave)) {
        ERROR("Cannot restore tty attributes");
        return -1;
    }

    return 0;
}


int 
stdin_noncannonical() {
    struct termios ttystate;

    // preserve  the terminal state
    if (tcgetattr(STDIN_FILENO, &ttystate)) {
        ERROR("Cannot get tty attributes");
        return -1;
    }
    ttysave = ttystate;
    ttystate.c_lflag &= ~(ICANON | ECHO);
    ttystate.c_cc[VMIN] = 1;
    ttystate.c_cc[VTIME] = 0;

    //set the terminal attributes.
    if (tcsetattr(STDIN_FILENO, TCSANOW, &ttystate)) {
        ERROR("Cannot set tty attributes");
        return -1;
    }

    dirty = true;
    return 0;
}

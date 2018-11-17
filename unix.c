#ifdef UNIX

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

static struct termios original_term;

int kbhit(void) {
    int bytesWaiting;
    ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

void setup_stdin(void) {
    struct termios term;
    tcgetattr(STDIN_FILENO, &original_term);
    term = original_term;
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void restore_stdin(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_term);
}

#endif
#ifdef UNIX

#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

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

void Ignore_quit_signal(void) {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
}

void Restore_quit_signal(void) {
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

void ms_sleep(unsigned int milliseconds) {
    int res;
    struct timespec t;
    t.tv_sec = milliseconds / 1000;
    t.tv_nsec = (milliseconds % 1000) * 1000000;

    do {
        res = nanosleep(&t, &t);
    } while (res == -1 && errno == EINTR);
}

#endif
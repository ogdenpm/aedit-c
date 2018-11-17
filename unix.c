#ifdef UNIX

#include <sys/ioctl.h>
#include <unistd.h>

int kbhit(void) {
    int bytesWaiting;
    ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

#endif
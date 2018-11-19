#ifdef UNIX

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "lit.h"
#include "type.h"
#include "proc.h"


static struct termios original_term;

int kbhit(void) {
    int bytesWaiting;
    ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

void setup_stdin(void) {
    struct termios term;
    tcgetattr(0, &original_term);
    term = original_term;
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &term);
    setbuf(stdin, NULL);
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


word access_rights = {0xffff};  /* 0FFFFH - invalid */

void Get_access_rights(pointer path_p) {
    
        byte str[string_len_plus_2];
        struct stat result;

    Move_name (path_p, str);
    str[str[0] + 1] = 0; /* convert to null-terminated */

    if (stat(&str[1], &result) != 0) {
        access_rights = 0xffff;  /* invalid */
    } else {
        access_rights = result.st_mode & 0777;
    }

} /* get_access_rights */



void Put_access_rights(pointer path_p) { 
        byte str[string_len_plus_2]; 

    if (access_rights == 0xffff)
        return;
    Move_name (path_p, str);
    str[str[0] + 1] = 0; /* convert to null-terminated */
    chmod (str + 1, access_rights & 0777);
    access_rights = 0xffff;

} /* put_access_rights */


#endif

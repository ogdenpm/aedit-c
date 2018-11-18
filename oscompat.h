#pragma once

#ifdef MSDOS
#define access _access
#define unlink _unlink
#define kbhit _kbhit

#else

#include <unistd.h>
#define _getch  getchar
#define _putch  putchar

int kbhit(void);
void setup_stdin(void);
void restore_stdin(void);
void Ignore_quit_signal(void);
void Restore_quit_signal(void);
void ms_sleep(unsigned int milliseconds);
#endif

#pragma once

#ifdef MSDOS
#define access _access
#define unlink _unlink
#define kbhit _kbhit
#else

#include <unistd.h>
int kbhit(void);

#endif

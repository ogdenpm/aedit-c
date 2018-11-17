#pragma once

#ifdef MSDOS
#define access _access
#define unlink _unlink
#define kbhit _kbhit
#define ms_sleep Sleep

#else

#include <unistd.h>
#include "unix.h"

#endif

#define DEFINE_CONSOLEV2_PROPERTIES
#include <windows.h>
#include <consoleapi.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>


pointer cmdLine;
pointer cmdLineStart;

bool EnableVTMode()
{
    COORD ssize = { 80, 25 };
    SMALL_RECT wsize = { 0, 0, 79, 24 };
    CONSOLE_SCREEN_BUFFER_INFOEX consoleScreenBufferInfoEx;
    consoleScreenBufferInfoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        return false;
    }


    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
 //   dwMode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
    if (!SetConsoleMode(hOut, dwMode))
    {
        return false;
    }
    return true;
}

void main(int argc, char **argv) {
    int len = 0;

    for (int i = 0; i < argc; i++)      // work out length of command line
        len += (int)(strlen(argv[i]) + 1);     // + 1 for space
    cmdLine = (pointer)malloc(len + 2); // add cr, lf and null - trailing space used for cr
    strcpy(cmdLine, argv[0]);           // start of command line
    for (int i = 1; i < argc; i++) {    // add other args each preceeded by a space
        strcat(cmdLine, " ");
        strcat(cmdLine, argv[i]);
    }
    strcat(strchr(cmdLine, 0) - 1, "\r\n");     // replace trailing space with cr lf null
    if (!EnableVTMode()) {
        fprintf(stderr, "can't enable virtual terminal\n");
        exit(1);
    }

    Aedit();
}

word findb(byte *str, byte ch, word cnt) {
    byte *s;
    s = (byte *)memchr(str, ch, cnt);
    if (s)
        return (word)(s - str);
    return 0xffff;

}


word findrb(byte *str, byte ch, word cnt) {
    byte *s;
    for (s = str + cnt - 1; s >= str; s--)
        if (*s == ch)
            return (word)(s - str);
    return 0xffff;

}

void movrb(pointer s, pointer t, word cnt) {
    s += cnt;
    t += cnt;
    while (cnt-- != 0)
        *--t = *--s;
}

word cmpb(pointer s, pointer t, byte cnt) {
    for (word i = 0; i < cnt; i++)
        if (*s++ != *t++)
            return i;
    return 0xffff;
}


word skipb(byte *str, byte ch, word cnt) {
    for (word i = 0; i < cnt; i++)
        if (*str++ != ch)
            return i;
    return 0xffff;
}


word findp(pointer src[], pointer p, word cnt) {    // was findw but used to look up pointers
    for (word i = 0; i < cnt; i++)
        if (src[i] == p)
            return i;
    return 0xffff;
}

void toCstr(char *cstr, byte *pstr) {   // convert pascal style string to c style string
    *cstr = 0;
    strncat(cstr, pstr + 1, *pstr);
}

// key udi emulation
void dq_rename(pointer old_p, pointer new_p, wpointer excep_p) {
    char old[FILENAME_MAX], new[FILENAME_MAX];

    toCstr(old, old_p);
    toCstr(new, new_p);

    *excep_p = rename(old, new) == 0 ? 0 : errno;


}

void dq_get_system_id(pointer id_p, wpointer excep_p) {       // id_p -> byte buf[21]
#ifdef MSDOS
    memcpy(id_p, "\x3WIN", 4);
#else
    memcpy(id_p, "\x4UNIX", 5);
#endif
    *excep_p = 0;
}



void dq_change_extension(pointer path_p, pointer extension_p, wpointer excep_p) {
    int cnt = *path_p < 4 ? *path_p : 4;        // chars to check
    pointer s = path_p + *path_p;                 // end char;

    while (cnt-- > 0 && *s != '.' && *s != '\\' && *s != '/')
        s--;
    
    if (*s != '.')
        s = path_p + *path_p + 1;               // no existing extension so append

    if (*extension_p != ' ') {                  // extension ?
        *s++ = '.';
        *s++ = *extension_p++;
        if (*extension_p != ' ') {
            *s++ = *extension_p++;
            if (*extension_p != ' ')
                *s++ = *extension_p;
        }
    }
    *path_p = (byte)(s - path_p - 1);                   // set the new length
}


bool isDelim(byte ch) {
    return ch <= ' ' || strchr(",()=#!$%`~+-;&|[]<>\x7f", ch) != NULL || ch >= 0x80;
}

// based on the DOS UDI implementation but note fixes problem with argument of 80 chars

static byte quote;          // used to carry over in quote status for long arguments - switch buffer should reset


byte dq_get_argument(pointer argument_p, wpointer excep_p) {
    
    int cnt = 0;
    word excep = E_OK;

    if (!quote)
        while (*cmdLine == ' ' || *cmdLine == '\t')
            *cmdLine++;

    while (excep == E_OK && *cmdLine && *cmdLine != '\r') {
        if (quote) {
            if (*cmdLine != quote || *++cmdLine == quote) {
                if (cnt != 80)
                    argument_p[++cnt] = *cmdLine++;
                else {
                    if (*cmdLine == quote)      // back up to double quote for next time
                        *cmdLine--;
                    excep = E_STRING_BUFFER;
                }
            } else
                quote = 0;

        } else if (*cmdLine == '\'' || *cmdLine == '"')
            quote = *cmdLine++;
        else if (isDelim(*cmdLine) && (*cmdLine != '-' || cnt == 0))    // allow embedded - in filenames
                break;
        else if (cnt == 80)
            excep = E_STRING_BUFFER;
        else
            argument_p[++cnt] = *cmdLine++;     // don't convert to upper case
    }
    *excep_p = excep;
    *argument_p = cnt;
    if (excep == E_OK) {
        while (*cmdLine == ' ' || *cmdLine == '\t')
            *cmdLine++;
        return isDelim(*cmdLine) ? *cmdLine++ : ' ';
    }
    return *cmdLine;
}

word dq_switch_buffer(pointer buffer_p, wpointer excep_p) {
    word off = (word)(cmdLine - cmdLineStart);
    cmdLine = cmdLineStart = buffer_p;
    quote = 0;
    *excep_p = E_OK;
    return off;
}

#define MAXFILES 16

struct {
    FILE *fp;
    byte flags;
    char name[FILENAME_MAX];
} _files[MAXFILES];






void dq_seek(connection conn, byte mode, dword offset, wpointer excep_p) {

    long pos = (long)offset;
    switch (mode) {
    case relative_back_mode:
        pos = -pos;           // negate offset to go backwards
    case relative_forward_mode: mode = SEEK_CUR; break;
    case abs_mode: mode = SEEK_SET; break;
    case end_minus_mode: mode = SEEK_END; break;
    }

    *excep_p = fseek(conn, pos, mode) == 0 ? 0 : errno;
}

void dq_delete(pointer path_p, wpointer excep_p) {
    char fname[FILENAME_MAX];
    toCstr(fname, path_p);
    *excep_p = _unlink(fname) == 0 ? 0 : errno;
}


struct {
    int code;
    char *errStr;
} udiExcep[] = {
    E_MEM, "Out of memory",
 //   E_FNEXIST, "File does not exists",
    E_FEXIST, "File already exists",
    E_STRING_BUFFER, "Argument > 80 chars",
    0, "Unknown Error"
};


/* decode exception using the os exception codes except for E_STRING_BUFFER */
void dq_decode_exception(word excep_code, pointer message_p, word maxLen) {
    char *msg;
    int i;

    if (excep_code < 128)
        msg = strerror(excep_code);
    else {
        for (i = 0; udiExcep[i].code != 0 && udiExcep[i].code != excep_code; i++)
            ;
        msg = udiExcep[i].errStr;
    }
    *message_p = sprintf(message_p + 1, "%04XH: %.*s", excep_code, maxLen - 7, msg);
}


void co_write(byte *buf, word len) {
    while (len-- > 0)
        _putch(*buf++);
}

static byte ci_mode = 1;

void set_ci_mode(byte mode) {
    ci_mode = mode;
}


struct {
    byte code;
    char *escSeq;
} keymap[] = {
    'H', "\033[A",      // up arrow
    'P', "\033[B",      // down arrow
    'M', "\033[C",      // right arrow
    'K', "\033[D",      // left arrow
    'G', "\033OP",      // home key
    'S', "\177",        // delete key (rubout)
    ';', "\033OP",      // PF1
    '>', "\033OS",      // PF4 (escape)
    '\0', ""
};

char *mapExtended(byte c1, byte c2) {   // ignore 0 / 0xe0 prefix
    int i = 0;
    while (keymap[i].code && keymap[i].code != c2)
        i++;
    return keymap[i].escSeq;

}

word ci_read(byte *buf) {
    byte c;
    static char *escSeq = "";

    while (1) {
        if (*escSeq) {
            *buf = *escSeq++;
            return 1;
        }
        switch (ci_mode) {
        case 3:
            if (_kbhit() == 0)   // if key then fall through
                return 0;
        case 1:
            c = _getch(); 
            if (c == 0 || c == 0xe0) {
                escSeq = mapExtended(c, _getch());
                continue;       // with either pick up escape sequence or retry
            }
            else
                *buf = c;
            return 1;
        default:
            return 0;
        }
    }
}



void sleep(word n) {
    // to implement
}

byte interface_buffer[100];
/* The strings in interface_buffer are null-terminated and length preceeded. */

pointer Getenv(pointer symbol_p) {
    /****************************************************************
       Gets a pointer to null terminated shell symbol, puts the value
       of that symbol in interface_buffer, and returns a pointer to it.
    ****************************************************************/

    char *str_p;

    if ((str_p = getenv(symbol_p)) == NULL)
        str_p = "";
    strcpy(interface_buffer + 1, str_p);
    interface_buffer[0] = (byte)strlen(str_p);
    return interface_buffer;

} /* getenv */

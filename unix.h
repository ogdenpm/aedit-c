#pragma once

int kbhit(void);
void setup_stdin(void);
void restore_stdin(void);
void Ignore_quit_signal(void);
void Restore_quit_signal(void);
void ms_sleep(unsigned int milliseconds);
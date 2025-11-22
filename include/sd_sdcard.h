#ifndef SDCARD_H
#define SDCARD_H

#include "sd_ff.h" // FatFs library header

void print_error(FRESULT fr, const char *msg);

void cd(int argc, char *argv[]);
void input(int argc, char *argv[]);
void ls(int argc, char *argv[]);
void mkdir(int argc, char *argv[]);
void mount(int argc, char *argv[]);
void pwd(int argc, char *argv[]);
void rm(int argc, char *argv[]);
void cat(int argc, char *argv[]);
void append(int argc, char *argv[]);
void date(int argc, char *argv[]);
void restart(int argc, char *argv[]);

void cat_hex(int argc, char *argv[]);
void play_wav(int argc, char *argv[]);
bool sd_cli_running(void);
void exit_cli(int argc, char *argv[]);

#endif
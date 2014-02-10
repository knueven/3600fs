/*
 * CS3600, Spring 2014
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#ifndef __3600FS_H__
#define __3600FS_H__

 #include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "disk.h"

#define UNUSED(x) (void)(x)

// Defining structures
typedef struct blocknum_t {
    int block:31;
    unsigned int valid:1;
} blocknum;

// Volume Control Block (VCB)
typedef struct vcb_s {
// a magic number of identify your disk
int magic; //777?
// description of the disk layout
int blocksize; //512
char name[504];
} vcb;

// Directory Entry
typedef struct dirent_s {
unsigned int valid;
unsigned int first_block;
unsigned int size;
uid_t user;
gid_t group;
mode_t mode;
struct timespec access_time;
struct timespec modify_time;
struct timespec create_time;
char name[497];
} dirent;

//FAT entry
typedef struct fatent_s {
unsigned int used:1;
unsigned int eof:1;
unsigned int next:30;
} fatent;

vcb *create_vcb(int magic);



#endif

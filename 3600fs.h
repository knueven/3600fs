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

#define UNUSED(x) (void)(x)
#define MAGIC 777 //magic number of the disk
#define DE_START 1
#define DE_LENGTH 100 //How many Directory entries do we have
#define FAT_START 101 //FAT starts at 101, right after Directory entires

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
    int de_start;
    int de_length;
    int fat_start; // block number where fat table starts
    int fat_length;// total number of blocks occupied by fat table
    int num_fatents;// total number of fat entries
    int db_start;
  
    // metadata for the root directory
    uid_t user;
    gid_t group;
    mode_t mode;
    struct timespec access_time;
    struct timespec modify_time;
    struct timespec create_time;
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
    char name[497]; //padding so sizeof(dirent) = 512 = BLOCKSIZE
} dirent;

//FAT entry
typedef struct fatent_s {
    unsigned int used:1;
    unsigned int eof:1;
    unsigned int next:30;
} fatent;

void startfat(int fatstart, int nfatblocks, int nfatents); 



#endif

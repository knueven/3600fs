/*
 * CS3600, Spring 2013
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#ifndef __3600FS_H__
#define __3600FS_H__

#include <sys/types.h>


//Structures defined here
typedef struct vcb_s {
  // a magic number of identify your disk
  int magic;
  
  // description of the disk layout
  int blocksize;
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

typedef struct dirent_s {
  unsigned int valid;
  int first_block;
  unsigned int size;
  uid_t user;
  gid_t group;
  mode_t mode;
  struct timespec access_time;
  struct timespec modify_time;
  struct timespec create_time;
  char name[28];
} dirent;

typedef struct fatent_s {
  unsigned int used:1;
  unsigned int eof:1;
  unsigned int next:30;
} fatent;

void startfat(int fatstart, int nfatblocks, int nfatents); 

/*
@function getnewfatent 
@return returns a newly allocated fat entry
*/
int getnewfatent();
/* @funcion allocatenfatents
*/
int allocatenfatents(int n, int *arr);

int printfats(int initial);
int removefats(int initial);

/*
@function writetodisk
Writes buflen number of bytes to the fatentry specified by value initial
Will allocate new blocks if it is required to do so
*/
int writetodisk(int initial, const char *buf, int buflen, int offset, int filelen);

int readfromdisk(int initial, char *buf, int buflen, int offset, int filelen);

/**
@function truncatefat
Will truncate the file, and free any blocks that need to be freed
*/
int truncatefat(int initial, int offset, int filelen);

void stopfat();

/* 
@funcion showfatstatus
Will display the status of fat table
*/
void showfatstatus();
#endif

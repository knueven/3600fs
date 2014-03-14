/*
 * CS3600, Spring 2013
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 * This program is intended to format your disk file, and should be executed
 * BEFORE any attempt is made to mount your file system.  It will not, however
 * be called before every mount (you will call it manually when you format 
 * your disk file).
 */

#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "3600fs.h"
#include "disk.h"

#define MAGIC 777 //Random number to identify disk
#define DE_START 1 //Where does Directory entries start, right after the VCB at 1
#define DE_LENGTH 100 //How many Directory entries do we have
#define FAT_START 101 //FAT starts at 101, right after Directory entires

void myformat(int size) {
  // Do not touch or move this function
  dcreate_connect();

  //Creating copy of vcb structure called myvcb and setting its values.
  vcb myvcb;
  myvcb.blocksize = BLOCKSIZE;
  myvcb.de_start = DE_START;
  myvcb.de_length = DE_LENGTH;
  myvcb.magic = MAGIC;

  int nblocks = size - 101; // this is the total blocks after dirent table
  // of these, some are fat table blocks, and rest are storage blocks
  myvcb.fat_length = nblocks / 128 + 1;  
  myvcb.fat_start = DE_LENGTH + 1;  
  myvcb.db_start = DE_LENGTH + myvcb.fat_length + 1;
  myvcb.num_fatents = size - myvcb.fat_start - myvcb.fat_length; // whatever remains after the fat tale is the total number of blocks; therefore, that is the number of fat entries in our blocks;

  fprintf( stderr, "fat start = %d fat length = %d num fatents = %d for size = %d\n", myvcb.fat_start, myvcb.fat_length, myvcb.num_fatents, size);

  // copy vcb to the first block
  char tmp[BLOCKSIZE];
  memset(tmp, 0, BLOCKSIZE);
  memcpy(tmp, &myvcb, sizeof(vcb));
  dwrite(0, tmp);

  //Creating copy of dirent structure called emptyde to write to disk
  dirent emptyde;
  emptyde.valid = 0; //Sets the valid value in emptyde to denote that the dirent is not in use
  char detmp[BLOCKSIZE] ;
  memset(detmp, 0, BLOCKSIZE);
  memcpy(detmp, &emptyde, sizeof(dirent));

  //For loop that copies data into DE entries
  for (int i=1; i<101; i++) {
    dwrite(i, detmp);
  }

  // create the blocks for fat table 
  memset(tmp, 0, BLOCKSIZE);
  for( int i = 0; i < myvcb.fat_length; i++) {
    dwrite ( myvcb.fat_start + i, tmp); 
  }
 

  // Do not touch or move this function
  dunconnect();
}

int main(int argc, char** argv) {
	// Do not touch this function
	if (argc != 2) {
	  printf("Invalid number of arguments \n");
	  printf("usage: %s diskSizeInBlockSize\n", argv[0]);
	  return 1;
  }

  unsigned long size = atoi(argv[1]);
	printf("Formatting the disk with size %lu \n", size);
	myformat(size);
}

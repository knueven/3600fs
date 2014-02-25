/*
 * CS3600, Spring 2014
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

//creates a VCB with the atributes set in 3600fs.h
vcb *create_vcb(int size) {
    vcb *s;
    //allocate memory and check that it worked
    s = (vcb *)calloc(1, sizeof(vcb));
    assert(s != NULL);

    //set magic number and block size
    s->magic = MAGIC;
    s->blocksize = BLOCKSIZE;

    //set the start/end of the directory entries
    s->de_start = DE_START; //1
    s->de_length = DE_LENGTH;
    int nblocks = size - 101; // this is the total blocks after dirent table
    // of these, some are fat table blocks, and rest are storage blocks
    s->fat_length = nblocks / 128 + 1;  
    s->fat_start = DE_LENGTH + 1;  
    s->db_start = DE_LENGTH + s->fat_length + 1;
    //only data blocks remain after the FAT table, therefore, that is the number of FAT entries
    s->num_fatents = nblocks - s->fat_start - s->fat_length; 

    return s;
}

dirent *create_dirent() {
  //Create an empty directory
  dirent *d;
  d = (dirent *)calloc(1, sizeof(dirent));
  assert(d != NULL);
  d->valid = 0; //0 = not in use (free)
  return d;
}

void myformat(int size) {
  // Do not touch or move this function
  dcreate_connect();

  /* 3600: FILL IN CODE HERE.  YOU SHOULD INITIALIZE ANY ON-DISK
           STRUCTURES TO THEIR INITIAL VALUE, AS YOU ARE FORMATTING
           A BLANK DISK.  YOUR DISK SHOULD BE size BLOCKS IN SIZE. */

  /* 3600: AN EXAMPLE OF READING/WRITING TO THE DISK IS BELOW - YOU'LL
           WANT TO REPLACE THE CODE BELOW WITH SOMETHING MEANINGFUL. */

  // first, create a zero-edout array of memory  
  char * tmp= (char *) malloc(BLOCKSIZE);
  memset(tmp, 0, BLOCKSIZE);

  // now, write that to every block
  for (int i=0; i<size; i++) 
    if (dwrite(i, tmp) < 0) 
      perror("Error while writing to disk");

  // voila! we now have a disk containing all zeros

  //and now create the VCB and write it to disk
  // set vcb data 
  vcb *v = create_vcb(size);
  
  //and write VCB to disk
  if (dwrite(0, (char *)v) < 0) {
    perror("Error while writing to disk");
  }
    //we're done with tmp
    free(tmp);

  dirent *tempde = create_dirent();

  //For loop that copies data into DE entries
  //will start at 1 and write to de_length + 1
  for (int i=1; i<DE_LENGTH+1; i++) {
    dwrite(i, (char *)tempde);
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

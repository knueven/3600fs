/*
 * CS3600, Spring 2014
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 * This file contains all of the basic functions that you will need 
 * to implement for this project.  Please see the project handout
 * for more details on any particular function, and ask on Piazza if
 * you get stuck.
 */

#define FUSE_USE_VERSION 26

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#define _POSIX_C_SOURCE 199309
#define MAGIC 777
#define UNUSED(x) (void)(x)

#include <time.h>
#include <fuse.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <sys/statfs.h>
#include <sys/stat.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "3600fs.h"
#include "disk.h"
#include "subdirs.h"
#include "subdirs.c"

//empty directory, for some testing
struct dirent_s emptyde;

//Global variables
vcb myvcb; //VCB
dirent dirents[100]; // Directory Entries

//Helper function
void write_dirent (int i) {
  char tmp[BLOCKSIZE];
  memset(tmp, 0, BLOCKSIZE);
  memcpy(tmp, &dirents[i], sizeof(dirent));
  dwrite(i+1,tmp);
  return;
}

/*
 * Initialize filesystem. Read in file system metadata and initialize
 * memory structures. If there are inconsistencies, now would also be
 * a good time to deal with that. 
 *
 * HINT: You don't need to deal with the 'conn' parameter AND you may
 * just return NULL.
 *
 */
static void* vfs_mount(struct fuse_conn_info *conn) {
  UNUSED(conn);
  fprintf(stderr, "vfs_mount called\n");

  // Do not touch or move this code; connects the disk
  dconnect();

  char tmp[BLOCKSIZE];
  memset(tmp, 0, BLOCKSIZE);
  dread (0, tmp); 
  memcpy(&myvcb, tmp, sizeof(vcb)); 

  //magic number checker
  if (myvcb.magic != MAGIC) {
    fprintf(stderr, "WRONG DISK - UNMOUNT NOW");
  }

  char detmp[BLOCKSIZE];
  for (int i=0; i<100; i++) {
    memset(detmp, 0, BLOCKSIZE); //Making detmp blocksize 0.
    dread (i + 1, detmp); //Reading i from detmp
    memcpy(&dirents[i], detmp, sizeof(dirent));//Copying detmp to dirents[i]
  }

  startfat( myvcb.fat_start, myvcb.fat_length, myvcb.num_fatents);
  return NULL;
}

/*
 * Called when your file system is unmounted.
 *
 */
static void vfs_unmount (void *private_data) {
  UNUSED(private_data);
  fprintf(stderr, "vfs_unmount called\n");

  //write all directory entries to disk
  for(int i = 0; i < 100; i++) {
    char buf[BLOCKSIZE];
    memset( buf, 0, BLOCKSIZE);
    memcpy( buf, &dirents[i], sizeof(dirents[i]));
    dwrite( 1 + i, buf);
  }
 
  //write fat table to disk too
  stopfat();


  // Do not touch or move this code; unconnects the disk
  dunconnect();
}

/* 
 *
 * Given an absolute path to a file/directory (i.e., /foo ---all
 * paths will start with the root directory of the CS3600 file
 * system, "/"), you need to return the file attributes that is
 * similar stat system call.
 *
 * HINT: You must implement stbuf->stmode, stbuf->st_size, and
 * stbuf->st_blocks correctly.
 *
 */
static int vfs_getattr(const char *path, struct stat *stbuf) {
  fprintf(stderr, "vfs_getattr called for %s\n", path);
  // Do not mess with this code 
  stbuf->st_nlink = 1; // hard links
  stbuf->st_rdev  = 0;
  stbuf->st_blksize = BLOCKSIZE;

  // home directory, or the root of the file system is a special case, 
  // because its entry does not exist in the dirents array
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode  = 0777 | S_IFDIR;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    return 0;
  }

  for(int i = 0; i < 100; i++) {
    if ((strcmp(dirents[i].name, path) == 0) && (dirents[i].valid == 1)) {
      fprintf( stderr, "In getattr: found the file %s in dirents\n", path);
    stbuf->st_uid =  dirents[i].user;
    stbuf->st_gid = dirents[i].group;
    //time structures
    struct tm * accesstm;
    struct tm * modifytm;
    struct tm * createtm;
    accesstm = localtime(&((dirents[i].access_time).tv_sec));
    modifytm = localtime(&((dirents[i].modify_time).tv_sec));
    createtm = localtime(&((dirents[i].create_time).tv_sec));

    stbuf->st_atime = mktime(accesstm);
    stbuf->st_mtime = mktime(modifytm);
    stbuf->st_ctime = mktime(createtm);

      stbuf->st_mode = dirents[i].mode;
    stbuf->st_size = dirents[i].size;
    if( dirents[i].size == 0) 
      stbuf->st_blocks = 1;//a special case cause we allocate a block just when a file is creted
    else
      stbuf->st_blocks  = ((dirents[i].size -1) /BLOCKSIZE)  + 1;
      return 0;
    }
  } //end of for loop
  
  // if control reaches here, it means the file did not exist
  fprintf(stderr, "In vfs_getattr, file not found, returning ENOENT");
  return -ENOENT;
}

/*
 * Given an absolute path to a directory (which may or may not end in
 * '/'), vfs_mkdir will create a new directory named dirname in that
 * directory, and will create it with the specified initial mode.
 *
 * HINT: Don't forget to create . and .. while creating a
 * directory.
 */
static int vfs_mkdir(const char *path, mode_t mode) {
  return 0;
} 

/** Read directory
 *
 * Given an absolute path to a directory, vfs_readdir will return 
 * all the files and directories in that directory.
 *
 * HINT:
 * Use the filler parameter to fill in, look at fusexmp.c to see an example
 * Prototype below
 *
 * Function to add an entry in a readdir() operation
 *
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stat file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @return 1 if buffer is full, zero otherwise
 * typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
 *                                 const struct stat *stbuf, off_t off);
 *         
 * Your solution should not need to touch fi
 *
 */
static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  UNUSED(fi);
  UNUSED(offset);
  char dirpath[128];
  strcpy(dirpath, path);
  // remove any trailing slash;strlen>1 cause we wont remove / for rootdir
  if( strlen(dirpath) > 1 && dirpath[strlen(dirpath)-1] == '/') 
     dirpath[strlen(dirpath)-1] = '\0';

  int i;
  fprintf( stderr, "In vfs_readdir: for path = %s\n", dirpath);

  filler( buf, ".", 0, 0);
  filler( buf, "..", 0, 0);
  for(i = 0; i < 100; i++) {
    if( dirents[i].valid == 1) {
      fprintf(stderr, "going to check isfilein for file = %s dir = %s\n", dirents[i].name, dirpath);
      if (isfilein( dirents[i].name, dirpath) ) {
         fprintf(stderr, "yes, file is in the given directory\n");
         struct stat stats;
         stats.st_mode = dirents[i].mode;        
         stats.st_uid = dirents[i].user;
         stats.st_gid = dirents[i].group;
         if( !strcmp(dirpath, "/"))
           filler( buf, dirents[i].name + 1, &stats,0);   
         else
           filler( buf, dirents[i].name + strlen(dirpath) + 1, &stats,0);
      }
    }  
  }

  return 0;
}

/*
 * Given an absolute path to a file (for example /a/b/myFile), vfs_create 
 * will create a new file named myFile in the /a/b directory.
 *
 */
static int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  UNUSED(fi);
  fprintf(stderr, "vfs_create called for %s\n", path);
  int i;
  int validdir = 0;
  char *dir = get_dir(path);
  if (dir == 0)
    return -1;

  // if the file is not in the root directory, first make sure that the 
  // directory in which it is being created actually exists
  if(strcmp( dir, "")) {
    for(i = 0; i < 100; i++) {
       if(!strcmp(dir, dirents[i].name) && (dirents[i].mode & S_IFDIR)) {
          // the directory exists; we can create the file
          validdir = 1;
          break;
       }
    }
    if (!validdir) {
      fprintf( stderr, "directory %s does not exist\n", dir);
      return -ENOTDIR; 
    }
  }
  
  //Checks to see if file already exists
  for(i = 0; i < 100; i++) {
    if (strcmp(dirents[i].name, path)==0) {
      return -EEXIST;
    }
  }

  for(i = 0; i < 100;  i++) { //Loops through all dirents till an empty dirent is found
    if (dirents[i].valid == 0) {
      fprintf(stderr, "In vfs_create: Found an empty dirent in %i, writing to it\n",i); //Tells us which block it is in
      dirents[i].valid = 1;
      dirents[i].first_block = getnewfatent();// allocate a fatent
      
      if( dirents[i].first_block == -1) {
         fprintf( stderr, "Error: no space on disk\n");
         return -1; // todo return the error code for running out of space
      }
      dirents[i].size = 0; // though a fatent is allocated, size is zero

      //Fill in getuid, getgid mode and path
      dirents[i].user = getuid();
      dirents[i].group = getgid();
      dirents[i].mode = mode;
      strcpy(dirents[i].name, path);
      clock_gettime(CLOCK_REALTIME, &dirents[i].access_time);
      clock_gettime(CLOCK_REALTIME, &dirents[i].modify_time);
      clock_gettime(CLOCK_REALTIME, &dirents[i].create_time);
      write_dirent(i);
      return 0;
    }
  }
  
  //Checks to see if you have any free space
  if(i == 100) { 
    fprintf(stderr, "Error: can't create file cause dirent is full\n");
    return -1; //You got to 100, which means you have no space!
  }
  
  return 0;
}

/*
 * The function vfs_read provides the ability to read data from 
 * an absolute path 'path,' which should specify an existing file.
 * It will attempt to read 'size' bytes starting at the specified
 * offset (offset) from the specified file (path)
 * on your filesystem into the memory address 'buf'. The return 
 * value is the amount of bytes actually read; if the file is 
 * smaller than size, vfs_read will simply return the most amount
 * of bytes it could read. 
 *
 * HINT: You should be able to ignore 'fi'
 *
 */
static int vfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
  UNUSED(fi);
  fprintf( stderr, "In vfs_read: path = %s size = %d offset = %d\n", path, size, offset);
  for (int i = 0; i < 100; i++) {
    if (strcmp(dirents[i].name, path) == 0 && dirents[i].valid == 1) {
      fprintf( stderr, "for file %s size is %d, and its fats are: \n", dirents[i].name, dirents[i].size);
      printfats( dirents[i].first_block);
      int readbytes = readfromdisk( dirents[i].first_block, buf, size, offset, dirents[i].size);
      fprintf( stderr, "\nin vfs_read: going to return %d\n", readbytes);
      return readbytes;
    }
  }
  return -ENOENT;
}

/*
 * The function vfs_write will attempt to write 'size' bytes from 
 * memory address 'buf' into a file specified by an absolute 'path'.
 * It should do so starting at the specified offset 'offset'.  If
 * offset is beyond the current size of the file, you should pad the
 * file with 0s until you reach the appropriate length.
 *
 * You should return the number of bytes written.
 *
 * HINT: Ignore 'fi'
 */
static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  UNUSED(fi);
  fprintf( stderr, "In vfs_write: path = %s size = %d offset = %d\n", path, size, offset);

  for (int i = 0; i < 100; i++) {
    if (strcmp(dirents[i].name, path) == 0 && dirents[i].valid == 1) {
      int newsize = writetodisk( dirents[i].first_block, buf, size, offset, dirents[i].size);
      if(newsize != -1) {
         dirents[i].size = newsize;
         fprintf(stderr, "write successfull, new file size = %d and  returning %d\nThe fats for the file are:", newsize, size);
         printfats( dirents[i].first_block);
         fprintf(stderr, "\n");
         
         return size;
      } else {
         return -1; // todo return error code for out of space
      }
    }
  }  

  return -ENOENT;
}

/**
 * This function deletes the last component of the path (e.g., /a/b/c you 
 * need to remove the file 'c' from the directory /a/b).
 */
static int vfs_delete(const char *path) {

  for (int i = 0; i < 100; i++) {
    if (strcmp(dirents[i].name, path) == 0 && dirents[i].valid == 1) {
      dirents[i].valid = 0;
      dirents[i].name[0] = '\0';
      removefats(dirents[i].first_block);
      write_dirent(i);
      return 0;
    }
  }
  
  // if control reaches here, that means the file did not exist
  return -ENOENT;
}

/*
 * The function rename will rename a file or directory named by the
 * string 'oldpath' and rename it to the file name specified by 'newpath'.
 *
 * HINT: Renaming could also be moving in disguise
 *
 */
static int vfs_rename(const char *from, const char *to) {
  fprintf(stderr, "vfs_rename calledon %s", from);
    for (int i = 0; i < 100; i++) {
      if (strcmp(dirents[i].name, to) == 0 && dirents[i].valid == 1) {
        vfs_delete(to);
      }
      if (strcmp(dirents[i].name, from) == 0 && dirents[i].valid == 1) {
        strcpy(dirents[i].name, to);
        return 0;
      }
      }
      //not found
      return -ENOENT;
}


/*
 * This function will change the permissions on the file
 * to be mode.  This should only update the file's mode.  
 * Only the permission bits of mode should be examined 
 * (basically, the last 16 bits).  You should do something like
 * 
 * fcb->mode = (mode & 0x0000ffff);
 *
 */
static int vfs_chmod(const char *file, mode_t mode) {
  for (int i = 0; i < 100; i++) {
    if (strcmp(dirents[i].name, file) == 0 && dirents[i].valid == 1) {
      dirents[i].mode = (mode & 0x0000ffff); 
      return 0;
    }
  }
   
  return -ENOENT;
}

/*
 * This function will change the user and group of the file
 * to be uid and gid.  This should only update the file's owner
 * and group.
 */
static int vfs_chown(const char *file, uid_t uid, gid_t gid) {
  for(int i = 0; i < 100; i++) {
        if (strcmp(dirents[i].name, file)==0) {
            dirents[i].user = uid;
            dirents[i].group = gid;
            return 0;
        }
      }
      return -1;
}

/*
 * This function will update the file's last accessed time to
 * be ts[0] and will update the file's last modified time to be ts[1].
 */
static int vfs_utimens(const char *file, const struct timespec ts[2]){
  for(int i = 0; i < 100; i++) {
        if (strcmp(dirents[i].name, file)==0) {
            dirents[i].access_time = ts[0];
            dirents[i].modify_time = ts[1];
            return 0;
        }
      }
      return -1;
}

/*
 * This function will truncate the file at the given offset
 * (essentially, it should shorten the file to only be offset
 * bytes long).
 *
 * On success, 0 is returned by truncate, as mentioned in man 
 * page of truncate(2)
 */
static int vfs_truncate(const char *file, off_t offset)
{
  
  for (int i = 0; i < 100; i++) {
    if (strcmp(dirents[i].name, file) == 0 && dirents[i].valid == 1) {
       
      if( dirents[i].first_block == -1)
        return -1;
      fprintf(stderr, "Before calling truncatefat, fats for file are:\n");
      printfats( dirents[i].first_block);
      int newsize = truncatefat(dirents[i].first_block, offset, dirents[i].size); 
      if(newsize == -1) {
        fprintf( stderr, "error with truncatefat, returning -1\n");
        return -1;
      }
     
      fprintf(stderr, "After calling truncatefat, fats for file are:\n");
      printfats( dirents[i].first_block);
      dirents[i].size = newsize;
      return 0;  
    }
  }
  return -ENOENT;
}


/*
 * You shouldn't mess with this; it sets up FUSE
 *
 * NOTE: If you're supporting multiple directories for extra credit,
 * you should add 
 *
 *     .mkdir  = vfs_mkdir,
 */
static struct fuse_operations vfs_oper = {
    .init    = vfs_mount,
    .destroy = vfs_unmount,
    .getattr = vfs_getattr,
    .readdir = vfs_readdir,
    .create  = vfs_create,
    .read  = vfs_read,
    .write   = vfs_write,
    .unlink  = vfs_delete,
    .rename  = vfs_rename,
    .chmod   = vfs_chmod,
    .chown   = vfs_chown,
    .utimens   = vfs_utimens,
    .truncate  = vfs_truncate,
    .mkdir       = vfs_mkdir,
};

/*
@variable fattable
Will contain all the fat entries in the file system. In other words, 
this will be a copy of the FAT table on the disk, but kept in memory. 
*/
fatent *fattable; 

int totalfatblocks;
int fatstartblock;
int totalfatents; // total fat entries in our system, and in fattable
int datastartblock; 

/*
@function startfat
Will start the fat system by loading it into ram. Is called on mount. 
On dismount, stopfat is called, which will write the in-memory fata to
the disk
*/
void startfat(int fatstart, int nfatblocks, int nfatents) {
  totalfatblocks = nfatblocks;
  fatstartblock = fatstart;
  datastartblock = fatstartblock + nfatblocks;
  totalfatents = nfatents; // this is also equal to the total number of blocks available for storage, because each storage block has one fatend
  

  fattable = (fatent*) malloc( nfatblocks * BLOCKSIZE); 

  for(int i = 0; i < nfatblocks; i++) { 
    char buf[BLOCKSIZE];
    memset(buf, 0, BLOCKSIZE);
    dread(fatstart + i, buf);
    //note: since fattable is fatent/int type, it requires use of 512/4=128
    memcpy( fattable + 128 * i, buf, BLOCKSIZE);   
  }

  return;
}

/*
@function getnewfatent 
@return returns a newly allocated fat entry
*/
int getnewfatent() {
  for(int i = 0; i < totalfatents; i++) {
     if( fattable[i].used == 0) {
        fattable[i].used = 1;
        fattable[i].eof = 1;// likely to be true, though the caller is the one responsible for the correctness of the value of .eof
        return i;
     }
  }
  return -1;
}
/**
@function allocatenfatents ( allocate n fatents)

Will return n number of newly allocated fatents by placing them in arr.
arr should already be allocated, and should be able to hold n fatents

@return returns the array, allocated with new fat ents; returns 0 on failure
*/
int allocatenfatents(int n, int* arr) {
  assert( n >= 0);
  assert( arr);  

  // now go over the fatents sequentially and try to allocate n unused ones
  int counter = 0;
  for(int i = 0; (i < totalfatents) && (counter < n); i++) {
     if( fattable[i].used == 0) {
        fattable[i].used = 1;
        arr[counter++] = i;
     }
  }

  if( counter < (n-1)) {
    fprintf( stderr, "Error: In allocatenfatents:no more space on disk\n"); 
    return 0;//means we have run out of fat entries; 
  }
  else
    return n; 
}

/**
@function addnewblocks
*/
int addnewblocks( int initial, int newblocks) {
  if (newblocks == 0) 
    return 0;

  int i = initial; 

  while( !fattable[i].eof) {
    i = fattable[i].next;
  }

  int *arr = (int *)malloc( newblocks * sizeof(int));
  int n = allocatenfatents( newblocks, arr); 
  if( n != newblocks)
    return -1;

  fattable[i].eof = 0;
  fattable[i].used = 1;
  fattable[i].next = arr[0];
  int j;
  for( j = 0; j < newblocks -1 ; j++) {
    fattable[arr[j]].eof = 0;   
    fattable[arr[j]].used = 1;  
    fattable[arr[j]].next = arr[j+1]; 
    //fprintf(stderr, "added new block %d\n", arr[j]);
  } 
  //fprintf(stderr, "added last new block %d\n", arr[j]);
  fattable[arr[j]].eof = 1;
  //fprintf(stderr, "block %d is eof\n", arr[j]);
  fattable[arr[j]].used = 1;

  return newblocks; 
} 

int removefats( int initial) {
  int i = initial; 
  
  while( !fattable[i].eof) {
    assert( fattable[i].used);
    fattable[i].used = 0; 
    i = fattable[i].next;
  }
  assert( fattable[i].used);
  fattable[i].used = 0;
  return 0;
}

/**
@function block 
Helper function that just tells in which block a byte would be 
The unit is in length, so 0-512 are in 0th block( the first block)
bytes 513-1024 are in block 1, etc

An offset is different than size of a file. offset 0 is at size 1; this
fact is useful when invoking this function
*/
int block(int lengthinbytes) {
  if(lengthinbytes == 0)
    return 0;
  else
    return (lengthinbytes-1 )/BLOCKSIZE ;
}



/**
@function writetochained 

This function assumes that all the blocks in the chain are properly allocated, and buf is to be written into these allocated blocks

*/ 

int writetochained( int initial, const char *buf, int buflen, int offset) {
  int offsetblock = block(offset+1); 
  int dsb = datastartblock;    

  int i = initial;
  int blocknum = 0;

  // we start by finding out the first int to which we have to write, 
  // which is the block that is sequentially equal to offsetblock 
  while( !fattable[i].eof ) {
    if( fattable[i].used == 0) {// something critical is wrong
      fprintf(stderr, "Error in fat.c::writeochained\n");
      return -1; 
    }
    if( blocknum == offsetblock) {
      break;
    } 
    blocknum++;
    i = fattable[i].next;
  }
  fprintf(stderr, "In writetochained: reached %d\n", i);

  if( blocknum != (offsetblock))
    return -1;// means eof was reached before we could start writing, which goes against the assumption of this function, that we have enough blocks allocated to write the buf
 
  // now to start writing 
  if( block( offset + 1) == block ( offset + buflen)) {
    // in this case, only one block needs to be written to
    char tmp[BLOCKSIZE];
    dread( dsb + i, tmp);
    memcpy( tmp + offset%BLOCKSIZE, buf, buflen); 
    dwrite(dsb + i, tmp);
  } else {
    // in this case, we need to write to more than one block, cause this 
    // is the case when block(offset) < block( offset + buflen), so the
    // buf spans multiple blocks
    char tmp[BLOCKSIZE];
    // write to first blcok
    int len = BLOCKSIZE - offset%BLOCKSIZE;
    fprintf(stderr, "In writetochained: writing %d bytes to block number %d\n", len, i);
    dread( dsb + i, tmp);
    memcpy( tmp + offset%BLOCKSIZE, buf, len); 
    dwrite(dsb + i, tmp);
    blocknum++; // this keeps track of the current block 
 
    i = fattable[i].next; // go to next block 
    int lastblock = block( offset + 1 + buflen); 
    fprintf(stderr, "next = %d blocknum = %d lastblock = %d\n", i, blocknum, lastblock );
    // now write to intermediate blocks, if any
    int j;
    for(j = 0; blocknum < lastblock; blocknum++, i = fattable[i].next, j++) {
      fprintf(stderr, "In writetochained: in for loop, writing a complete block to block id %d; blocknum = %d\n", i, blocknum);
      dwrite(dsb + i, (char*)buf + (j + 1) * BLOCKSIZE - offset%BLOCKSIZE);
    }
       
    // and write to last block 
    dread( dsb + i, tmp);
    len = (offset+buflen)%BLOCKSIZE; 
    memset(tmp, 0, BLOCKSIZE);
    memcpy( tmp, buf + (j+1) *BLOCKSIZE - offset%BLOCKSIZE, len);
    fprintf(stderr, "In writetochained: writing %d bytes to last block %d\nfirst byte written is \"%c\"", len, i, tmp[0]);
    dwrite(dsb + i, tmp);
    // just write the eof 
    fattable[i].eof = 1;
  }
  
  return buflen;
}

/*
@function writetodisk
Writes buflen number of bytes to the fatentry specified by value initial
Will allocate new blocks if it is required to do so
*/
int writetodisk(int initial, const char *buf, int buflen, int offset, int filelen) {
  if( initial == -1 || buf == 0) {
    return -1;
  } 

  if( buflen == 0) {
    return 0; 
  }
 
  int dsb = datastartblock;    // using a shorter name
  fprintf(stderr, "in writetodisk; buflen = %d offset = %d filelen = %d\n", buflen, offset, filelen); 
  // fine current number of file blocks using filelen


  // check if we require to allocate more blocks 
  if(filelen == 0) {//filelen==0 is a special case, cause a block is allocated already
    if( buflen <= BLOCKSIZE) {// we can just write to the 1st block
      char tmp[BLOCKSIZE];
      memset(tmp, 0, BLOCKSIZE);
      memcpy( tmp + offset, buf, buflen);
      dwrite( dsb + initial, tmp);
      fattable[initial].used = 1; // just to be on the safe side
      fattable[initial].eof = 1; // cause this is the end of file
      return offset + buflen; // this is the new size of the file
    } else {
      // we need to allocate blocks for (buflen - 512) bytes; its -512 cause
      // the first block is already allocated and ready to be used
      int nblocksrequired = (buflen- BLOCKSIZE - 1)/512 + 1;
      int *allocatedblocks = (int *) malloc( nblocksrequired * sizeof (int));
      int numallocatedblocks = allocatenfatents( nblocksrequired, allocatedblocks); 
      fprintf(stderr, "allocated %d blocks\n", numallocatedblocks);
      assert( numallocatedblocks == nblocksrequired);//just making sure
   
      //copy initial BLOCKSIZE bytes to the first block, which was already allocated
      dwrite( dsb + initial, (char *)buf);// dwrite will just pick BLOCKSIZE bytes  
      // now copy buffer starting at buf + 512, of lenght buflen-512 
      // also set the respective .used and .eof flags
 
      fattable[initial].used = 1;
      fattable[initial].eof = 0; 
      fattable[initial].next = allocatedblocks[0];
      fprintf( stderr, "Write BLOCKSIZE to first initial block %d\n", initial);
      // we copy the buffer by copyintg n-1 full blocks, and then the last
      // blocks is partially(or fully) copied
 
      for(int i = 0; i < numallocatedblocks; i++) {
        fattable[ allocatedblocks[i]].used = 1;
        if( i < (numallocatedblocks-1)) {
          fprintf( stderr, "writing full block number %d\n", allocatedblocks[i]);
          // there is a succeeding block for this block, so set the fields
          // accordingly
          fattable[ allocatedblocks[i]].next = allocatedblocks[i+1];
          fattable[ allocatedblocks[i]].eof = 0;
          // now copy the full buffer; 
          dwrite(dsb + allocatedblocks[i], (char *)(buf + BLOCKSIZE + i * BLOCKSIZE));
        } 
        else {
          // this is the last block in the chain
          fattable[ allocatedblocks[i]].next = 0;
          //fprintf(stderr, "block %d is eof\n", allocatedblocks[i]);
          fattable[ allocatedblocks[i]].eof = 1;
          // copy buffer;in this case the buffer may be less than BLOCKSIZE
          int len = buflen - BLOCKSIZE - i * BLOCKSIZE; 
          char tmp[BLOCKSIZE];
          memset( tmp, 0, BLOCKSIZE);
          memcpy( tmp, buf + BLOCKSIZE + i * BLOCKSIZE, len); 
          dwrite( dsb + allocatedblocks[i], tmp);
          fprintf( stderr, "writing partially, block number %d bytes = %d first byte writtten = \"%c\"\n", allocatedblocks[i], len, buf[ BLOCKSIZE + i * BLOCKSIZE]);
        }
      } // end for loop which writes to blocks
    }
    return offset + buflen;
  }
  else if( filelen < buflen + offset) {
    // in this case we MAY need to allocate more blocks
    if(block(filelen) < block( offset + buflen )) {
       // this is the case when we need more blocks, equal to "newblocks"
       int newblocks = block(offset + buflen) - block( filelen); 

       fprintf(stderr,"In write to disk4: allocating %d new blocks\n"
                      "block(offset + buflen) = %d block(filelen) %d\n",
                       newblocks, block( offset+buflen), block(filelen));
      
       addnewblocks(initial, newblocks);//this will allocate our new blocks 
    
       // there are two cases here: if offset starts after filelen,then
       // we are required to pad the intermediate space with zeros
       if( offset > filelen) {
         // pad with zeros
       } else {
         // nothing needs to be done here
       }

       // now we have allocated blocks, and padded if it was required, so
       // we are ready to copy the buf to the allocated bytes
       writetochained( initial, buf, buflen, offset ); 
       return buflen + offset; // this is the new size of the file
    } else {
      // the extra bytes can be copied in the allocated blocks
      writetochained(initial, buf, buflen, offset); 
      return buflen + offset; // the old size remains
    }   

  }  else {
    // no need to allocate more blocks
    writetochained(initial, buf, buflen, offset); 
    return filelen;
  }
     

}

int readfromdisk(int initial, char *buf, int buflen, int offset, int filelen) {
  if( offset >= filelen) {
    return -1;
  }

  const int dsb = datastartblock;
  int startblock = block(offset+1); 
  int endblock;

  if( filelen < offset + buflen) {
    buflen = filelen - offset;
  } 
 
  endblock = block( offset + buflen);

  int counter = 0;
  int i = initial;
  while(!fattable[i].eof) {
    
    if( counter == startblock)
      break; 
  
    i = fattable[i].next; 
    fprintf(stderr, "In readfromdisk: skipping over fat num %d\n",i);
    counter++;
  }  
  fprintf(stderr, "In readfromdisk: startblock = %d endblock = %d i = %d\n", startblock, endblock, i);
  int alreadyread = 0; 
  int readlen = buflen;
  if( offset + buflen > filelen)
    readlen = filelen - offset; 

  int toread = readlen;
  if(offset%BLOCKSIZE + readlen > BLOCKSIZE) {
    // this means we have to read more than one block
    fprintf( stderr, "readfromdisk: going to read from more than one block\n");
    // read the first block
    char tmp[BLOCKSIZE];
    memset(tmp,0, BLOCKSIZE);
    dread( dsb + i, tmp);
    memcpy( buf, tmp + offset%BLOCKSIZE, BLOCKSIZE - offset%BLOCKSIZE);
    alreadyread = BLOCKSIZE - offset%BLOCKSIZE;   
    i = fattable[i].next; // important part, to make sure we read the next blk
    counter++; // keeps count of number of blocks read
 
    // the remaining bytes to be read
    readlen = readlen - alreadyread; 
     
         
    // now go over the rest of the blocks
    while(counter<=endblock) {
      fprintf(stderr, "In while: i = %d counter = %d\n", i, counter );
      if( fattable[i].used != 1)
        return -1; // something wrong here, so quit from the function
      
      if(readlen <= BLOCKSIZE) { // the last iteration of the while loop
        // read just this block now, then break from loop
        memset(tmp,0, BLOCKSIZE);
        dread( dsb + i, tmp);
        memcpy( buf + alreadyread, tmp, readlen);
        fprintf( stderr, "read readlen = %d bytes from last block = %d; first byte read is \"%c\" at alreadyread = %d\n", readlen, i, buf[alreadyread ], alreadyread );
        alreadyread += readlen;
        break; 
      } else {
        memset(tmp,0, BLOCKSIZE);
        dread( dsb + i, tmp);
        fprintf(stderr, "alreadyread = %d readlen = %d\n", alreadyread, readlen);
        memcpy( buf + alreadyread, tmp, BLOCKSIZE);
        alreadyread += BLOCKSIZE;
        readlen -= BLOCKSIZE;
      }
      fprintf(stderr, "In readfromdisk: read from block i = %d, next = %d\n", i, fattable[i].next);
      i = fattable[i].next;
      counter++;
    }
    
    if( toread != alreadyread) {
      fprintf( stderr, "toread = %d alreadyread =%d and are not equal\n", toread, alreadyread);
      assert(0);
    }
    return alreadyread; 
  } else {
    // read just one block, the current one (i)
    char tmp[BLOCKSIZE];
    memset(tmp,0, BLOCKSIZE);
    dread( dsb + i, tmp);
    memcpy( buf, tmp + offset%BLOCKSIZE, readlen);
    return readlen;   
  }

  return 0;
}

int truncatefat( int initial, int offset, int filelen) {
  
  if( offset > filelen)
    return -1;


  if(block(offset) == block( filelen)) {
    // in this case we dont need to free any block
    return offset;
  } else if( block(offset + 1) < block(filelen)) {
    // in this case, we need to free at least one block
       
    // find the int in which offset exists
    int offsetblock = block(offset +1); 
    int i = initial;
    int counter = 0;
    while(!fattable[i].eof) {
      if( fattable[i].used == 0)
        return -1; 
  
      if( counter == offsetblock)
        break;
  
      i = fattable[i].next;
      counter++;
    }
    
    // now i is the int after which we have to free all 
    if(fattable[i].eof == 1) {
      fprintf( stderr, "In truncatefat: error somewhere, returning -1\n");
      return -1;
    }
    
    fattable[i].eof = 1; // this will be the last block now  
    fprintf(stderr, "block %d is eof\n", i);
    i = fattable[i].next;
    while(!fattable[i].eof) { 
      fattable[i].used = 0;
      i = fattable[i].next;
      fprintf( stderr, "In truncatefat: removed a block %d\n", i);
    }
    fattable[i].used = 0;
    fattable[i].eof = 0;
    return offset;
  } 
  
  // block(offset) > block(filelen) is not possible because we 
  // already checked for it in the initial if condition
 
  return -1;
}
/*
@funcion stopfat

The major or only job of this function is to write the in-memory fat table
to disk. It does so one block at a time, as is allowed by our disk.
*/
void stopfat() {
  
  
  for(int i = 0; i < totalfatblocks; i++) { 
    char buf[BLOCKSIZE];
    memcpy( buf, fattable + 128 * i, BLOCKSIZE);//requires 128 cause sizeof(fattable) is 4
    dwrite(fatstartblock + i, buf);
  }
 
  return;
}

int printfats( int initial) {
 
  int i = initial; 
  
  while(!fattable[i].eof) {
    fprintf( stderr, "%d ", i);
    i = fattable[i].next;
  }
  fprintf( stderr, "%d", i);

  return 0;
}

/* 
@funcion showfatstatus
Will display the status of fat table
*/
void showfatstatus() {
  fprintf(stderr, "FAT status\n---------------------\nThe following blocks are in use\n");
  for(int i = 0; i < totalfatents; i++) {
    if( fattable[i].used)
      fprintf(stderr, "%d, ", i);
    }
  fprintf(stderr, "\n");
  return;  
}



int main(int argc, char *argv[]) {
    /* Do not modify this function */
    umask(0);
    if ((argc < 4) || (strcmp("-s", argv[1])) || (strcmp("-d", argv[2]))) {
      printf("Usage: ./3600fs -s -d <dir>\n");
      exit(-1);
    }
    return fuse_main(argc, argv, &vfs_oper, NULL);
}

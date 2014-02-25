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


 // Global VCB
vcb v;
char * disk_status;
int num_blocks;

vcb getvcb(){
  vcb vb;
  char temp_vb[BLOCKSIZE];
  memset(temp_vb, 0, BLOCKSIZE);
  if (dread(0, temp_vb) < 0) {
	fprintf(stderr, "Error reading VCB\n");
	}
  memcpy(&vb, temp_vb, sizeof(vcb));
  return vb;
}

dirent getdirent(int block) {
  dirent dir;
  char temp_dir[BLOCKSIZE];
  memset(temp_dir, 0, BLOCKSIZE);
  if (dread(block, temp_dir) < 0) {
    fprintf(stderr, "Error reading dirent at block %d\n", block);
  }
  memcpy(&dir, temp_dir, sizeof(dirent));
  return dir;
}

int write_dirent(int block, dirent dir) {
  char tmp[BLOCKSIZE];
  memset(tmp, 0, BLOCKSIZE);
  memcpy(tmp, &dir, BLOCKSIZE);
  if (dwrite(block, tmp) != BLOCKSIZE) {
    fprintf(stderr, "error writing dirent");
  }
return 512;
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

  vcb v = getvcb();
  //if we're dealing with the wrong disk. you should unmount if this error is thrown
  if (v.magic != MAGIC) {
    fprintf(stderr, "Wrong disk, magic # is incorrect\n");
  }
  dirent d = getdirent(1);
  fprintf(stderr, "dirent %d\n", d.valid);
  startfat(v.fat_start, v.fat_length, v.num_fatents);
  return NULL;
}

/*
 * Called when your file system is unmounted.
 *
 */
static void vfs_unmount (void *private_data) {
  UNUSED(private_data);
  fprintf(stderr, "vfs_unmount called\n");

  /* 3600: YOU SHOULD ADD CODE HERE TO MAKE SURE YOUR ON-DISK STRUCTURES
           ARE IN-SYNC BEFORE THE DISK IS UNMOUNTED (ONLY NECESSARY IF YOU
           KEEP DATA CACHED THAT'S NOT ON DISK */

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
  fprintf(stderr, "vfs_getattr called on %s\n", path);
  
  // Do not mess with this code 
  stbuf->st_nlink = 1; // hard links
  stbuf->st_rdev  = 0;
  stbuf->st_blksize = BLOCKSIZE;

  //if (The path represents the root directory)
  if (strcmp(path, "/") == 0) {
      stbuf->st_mode  = 0777 | S_IFDIR;
      stbuf->st_uid = getuid();
      stbuf->st_gid = getgid();
      return 0;
  }
  for(int i = 1; i < DE_LENGTH + 1; i++) {
    //read the dirent at block i
    dirent mydirent = getdirent(i);
    if ((strcmp(mydirent.name, path) == 0) && (mydirent.valid == 1)) {
      fprintf( stderr, "In getattr: found the file %s in dirent %d\n", path, i);
    stbuf->st_uid =  mydirent.user;
    stbuf->st_gid = mydirent.group;
    //Creates pointers to structures so I can use them to get time
    struct tm * acctm; //access time
    struct tm * modtm; //modify time
    struct tm * cretm; //created time
    //localtime comverts tv_sec from timespec into a struct tm
    acctm = localtime(&((mydirent.access_time).tv_sec));
    modtm = localtime(&((mydirent.modify_time).tv_sec));
    cretm = localtime(&((mydirent.create_time).tv_sec));
    //and mktime makes sure things like the 40th of a month don't happen
    stbuf->st_atime = mktime(acctm);
    stbuf->st_mtime = mktime(modtm);
    stbuf->st_ctime = mktime(cretm);

    //set mode and size
    stbuf->st_mode = mydirent.mode;
    stbuf->st_size = mydirent.size;
    if( mydirent.size == 0) 
      stbuf->st_blocks = 1;//a special case cause we allocate a block just when a file is creted
    else {
      stbuf->st_blocks  = ((mydirent.size -1) /BLOCKSIZE)  + 1;
    }
      return 0;
    }
  } //end of for loop
  
  // if control reaches here, it means the file did not exist
  fprintf(stderr, "File not found by vfs_getattr");
  return -ENOENT;

  return 0;
}

/*
 * Given an absolute path to a directory (which may or may not end in
 * '/'), vfs_mkdir will create a new directory named dirname in that
 * directory, and will create it with the specified initial mode.
 *
 * HINT: Don't forget to create . and .. while creating a
 * directory.
 */
/*
 * NOTE: YOU CAN IGNORE THIS METHOD, UNLESS YOU ARE COMPLETING THE 
 *       EXTRA CREDIT PORTION OF THE PROJECT.  IF SO, YOU SHOULD
 *       UN-COMMENT THIS METHOD.
static int vfs_mkdir(const char *path, mode_t mode) {

  return -1;
} */

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
static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
  int i;
  fprintf( stderr, "In vfs_readdir: for path = %s\n", path);

  filler( buf, ".", 0, 0);
  filler( buf, "..", 0, 0);
  for(i = 1; i < DE_LENGTH + 1; i++) {
    dirent dir = getdirent(i);
    if( dir.valid == 1) {
         struct stat stats;
         stats.st_mode = dir.mode;        
         stats.st_uid = dir.user;
         stats.st_gid = dir.group;
         if( !strcmp(path, "/"))
           filler( buf, dir.name + 1, &stats,0);   
         else
           filler( buf, dir.name + strlen(path) + 1, &stats,0);
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
     fprintf(stderr, "vfs_create called for %s\n", path);
     int i = 0;
  //Checks to see if file already exists
  for(i = 1; i < DE_LENGTH + 1; i++) {
    dirent dx = getdirent(i);
    if (strcmp(dx.name, path)==0) {
      return -EEXIST;
    }
  }

  for(i = 1; i < DE_LENGTH + 1;  i++) { //Loops through all dirents till an empty dirent is found
    dirent dir = getdirent(i);
    if (dir.valid == 0) {
      fprintf(stderr, "In vfs_create: Found an empty dirent in %i, writing to it\n",i); //Tells us which block it is in
      dir.valid = 1;
      dir.first_block = getnewfatent();// allocate a fatent
      
      if( dir.first_block == -1) {
         fprintf( stderr, "Error: no space on disk\n");
         return -1; // todo return the error code for running out of space
      }
      dir.size = 0; // though a fatent is allocated, size is zero

      //Fill in getuid, getgid mode and path
      dir.user = getuid();
      dir.group = getgid();
      dir.mode = mode;
      strcpy(dir.name, path);
      clock_gettime(CLOCK_REALTIME, &dir.access_time);
      clock_gettime(CLOCK_REALTIME, &dir.modify_time);
      clock_gettime(CLOCK_REALTIME, &dir.create_time);

    //and now write the dirent to disk
    write_dirent(i, dir);
    return 0;
  }
}

  //Checks to see if you have any free space
  if(i == DE_LENGTH + 1) { 
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

    return 0;
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
static int vfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{

  /* 3600: NOTE THAT IF THE OFFSET+SIZE GOES OFF THE END OF THE FILE, YOU
           MAY HAVE TO EXTEND THE FILE (ALLOCATE MORE BLOCKS TO IT). */

  return 0;
}

/**
 * This function deletes the last component of the path (e.g., /a/b/c you 
 * need to remove the file 'c' from the directory /a/b).
 */
static int vfs_delete(const char *path)
{

  for (int i = 1; i < DE_LENGTH + 1; i++) {
    dirent dir = getdirent(i);
    if (strcmp(dir.name, path) == 0 && dir.valid == 1) {
      dir.valid = 0;
      dir.name[0] = '\0';
      removefats(dir.first_block);
      write_dirent(i, dir);
      return 0;
    }
  }

  //if control gets here, file did not exist
  return -ENOENT;

}

/*
 * The function rename will rename a file or directory named by the
 * string 'oldpath' and rename it to the file name specified by 'newpath'.
 *
 * HINT: Renaming could also be moving in disguise
 *
 */
static int vfs_rename(const char *from, const char *to)
{
  fprintf(stderr, "vfs_rename called on %s", from);
  for (int i = 1; i < DE_LENGTH + 1; i++) {
    dirent dir = getdirent(i);
    if (strcmp(dir.name, to) == 0 && dir.valid == 1) {
      vfs_delete(to);
    }
    if (strcmp(dir.name, from) == 0 && dir.valid == 1) {
      strcpy(dir.name, to);
      write_dirent(i, dir);
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
static int vfs_chmod(const char *file, mode_t mode)
{
    chmod(file, mode);
    return 0;
}

/*
 * This function will change the user and group of the file
 * to be uid and gid.  This should only update the file's owner
 * and group.
 */
static int vfs_chown(const char *file, uid_t uid, gid_t gid)
{

    return 0;
}

/*
 * This function will update the file's last accessed time to
 * be ts[0] and will update the file's last modified time to be ts[1].
 */
static int vfs_utimens(const char *file, const struct timespec ts[2])
{

    return 0;
}

/*
 * This function will truncate the file at the given offset
 * (essentially, it should shorten the file to only be offset
 * bytes long).
 */
static int vfs_truncate(const char *file, off_t offset)
{

  /* 3600: NOTE THAT ANY BLOCKS FREED BY THIS OPERATION SHOULD
           BE AVAILABLE FOR OTHER FILES TO USE. */

    return 0;
}


/*
 * You shouldn't mess with this; it sets up FUSE
 *
 * NOTE: If you're supporting multiple directories for extra credit,
 * you should add 
 *
 *     .mkdir	 = vfs_mkdir,
 */
static struct fuse_operations vfs_oper = {
    .init    = vfs_mount,
    .destroy = vfs_unmount,
    .getattr = vfs_getattr,
    .readdir = vfs_readdir,
    .create	 = vfs_create,
    .read	 = vfs_read,
    .write	 = vfs_write,
    .unlink	 = vfs_delete,
    .rename	 = vfs_rename,
    .chmod	 = vfs_chmod,
    .chown	 = vfs_chown,
    .utimens	 = vfs_utimens,
    .truncate	 = vfs_truncate,
};

/*
fattable
will contain all the fat entries in the file system. In other words, 
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

int removefats(int initial) {
  int i = initial; 
  
  while( !fattable[i].eof) {
    assert( fattable[i].used);
    fattable[i].used = 0; 
    i = fattable[i].next;
  }
  assert(fattable[i].used);
  fattable[i].used = 0;
  return 0;
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


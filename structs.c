#include "3600fs.h"

//the magic number for our disk
const int MAGIC = 777;

//size represents the number of blocks on the disk
vcb *create_vcb(int size) {
    vcb *s;
    //allocate memory and check that it worked
    s = (vcb *)calloc(1, sizeof(vcb));
    assert(s != NULL);

    //set magic number and block size
    s->magic = MAGIC;
    s->blocksize = BLOCKSIZE;

    //set the start/end of the directory entries
    s->de_start = 1;
    s->de_length = 100;

    return s;
}
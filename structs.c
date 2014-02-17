#include "3600fs.h"


const int MAGIC = 777;

vcb *create_vcb() {
    vcb *s;
    //allocate memory and check that it worked
    s = (vcb *)calloc(1, sizeof(vcb));
    assert(s != NULL);

    //set magic number and block size
    s->magic = MAGIC;
    s->blocksize = BLOCKSIZE;

    //set the start/end of the directory entries
    s->de_start = 1;
    s->de_end = 101;

    return s;
}
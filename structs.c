#include "3600fs.h"


const int MAGIC = 777;

vcb *create_vcb(int magic) {
    vcb *s;
    s = (vcb *)calloc(1, sizeof(vcb));
    assert(s != NULL);

    s->magic = magic;
    s->blocksize = BLOCKSIZE;


    return s;
}
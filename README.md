3600fs
======

For this project, we had to implement a simple user-level filesystem using 
a block interface to a raw disk

###Design Choice###

We went with a **FAT (File Allocation Table)** filesystem, which uses an index table that contains entries for each contiguous area of disk storage.
We chose FAT because of its robustness and simplicity, as it easily enables the OS to read data as a sequence of blocks defined in the file allocation table.

#####*FAT Implementation*#####

To implement the filesystem using FAT, the disk is split into 4 different types of blocks:
  - **Volume Control Block (VCB)**
    * First block in the filesystem
    * Contains the information required to find the remainder of the filesystem structure.
  - **Directory Entry (DE)**
    * Represents a file in the filesystem
    * Contains metadata about the file
    * For our filesystem, this is where the maximum number of files supported is defined
  - **File Allocation Table (FAT)**
    * These are a series of pointers that say which data blocks which in the layout of the files.
    * Contains an entry for each data block
  - **Data Block (DB)**
    * This is where data is actually stored

###Implementation Notes###
- For *free space* to minimize overhead, and redundancy, every free DB is initialized to point to the next free block.
  Therefore, when the filesystem is initialized, the DB's are initialized as a *linked list* of sorts, where every DB     has a pointer to the next free DB
- Other than the DB, all the other block types are overhead, since they only contain metadata about the files, rather     than the data itself.

      

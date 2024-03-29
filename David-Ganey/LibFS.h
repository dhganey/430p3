#ifndef __LibFS_h__
#define __LibFS_h__

/*
 * DO NOT MODIFY THIS FILE
 */
    
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

const int NUM_INODES = 1000;
const int NUM_CHARS = 125;
const int NUM_DATA_BLOCKS = 746;
const int MAX_FILES = 1000;
const int NUM_DIRECTORIES_PER_BLOCK = 16;
const int NUM_INODES_PER_BLOCK = 4;
const int NUM_POINTERS = 30;

//sector "pointer" offsets
const int SUPER_BLOCK_OFFSET = 0;
const int INODE_BITMAP_OFFSET = 1;
const int DATA_BITMAP_OFFSET = 2;
const int ROOT_INODE_OFFSET = 3; //skip 1 for data bitmap 2
const int FIRST_DATABLOCK_OFFSET = 255;

// used for errors
extern int osErrno;
    
// error types - don't change anything about these!! (even the order!)
typedef enum {
    E_GENERAL,      // general
    E_CREATE, 
    E_NO_SUCH_FILE, 
    E_TOO_MANY_OPEN_FILES, 
    E_BAD_FD, 
    E_NO_SPACE, 
    E_FILE_TOO_BIG, 
    E_SEEK_OUT_OF_BOUNDS, 
    E_FILE_IN_USE, 
    E_BUFFER_TOO_SMALL, 
    E_DIR_NOT_EMPTY,
    E_ROOT_DIR,
} FS_Error_t;
    
// File system generic call
int FS_Boot(char *path);
int FS_Sync();

// file ops
int File_Create(char *file);
int File_Open(char *file);
int File_Read(int fd, void *buffer, int size);
int File_Write(int fd, void *buffer, int size);
int File_Seek(int fd, int offset);
int File_Close(int fd);
int File_Unlink(char *file);

// directory ops
int Dir_Create(char *path);
int Dir_Size(char *path);
int Dir_Read(char *path, void *buffer, int size);
int Dir_Unlink(char *path);

//helper functions

#endif /* __LibFS_h__ */

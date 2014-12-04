#include "LibFS.h"
#include "LibDisk.h"

#include <string>
#include <vector>
#include <iostream>
#include <math.h>

// global errno value here
int osErrno;

//structs

typedef struct superblock
{
    char magic[3];
} Superblock;

typedef struct inode
{
    int fileType; //0 represents file, 1 is a directory
    int fileSize; //in bytes
    int pointers[30];
} Inode;

typedef struct directoryentry
{
    char name[16];
    int inodeNum;
    char garbage[12];
} DirectoryEntry;

typedef struct bitmap
{
    char bits[NUM_CHARS];
    char garbage[SECTOR_SIZE - NUM_CHARS];
} Bitmap;

Bitmap* inodeBitmap;
Bitmap* dataBitmap;

//============ Helper Functions ============
int
Create_New_Disk(char* path)
{
    int ok = 0;

    //prep the superblock
    //superblock contains "666" followed by garbage
    ok = Disk_Write(SUPER_BLOCK_OFFSET, magicString);
    if (ok == -1)
    {
        osErrno = E_CREATE;
        return ok;
    }
    
   //TODO initialize and write the bitmaps

    //create the root directory
    ok = Dir_Create("/");
    if (ok == -1)
    {
        osErrno = E_CREATE;
        return ok;
    }

    ok = Disk_Save(path);
    if (ok == -1)
    {
        osErrno = E_CREATE;
        return ok;
    }

    return ok;
}

//Finds the first 0 in the inode bitmap
int findFirstAvailableInode()
{
    //Look through each char
    for (int i = 0; i < NUM_CHARS; i++)
    {
        char c = inodeBitmap->bits[i];
        //Each char represents 8 bits
        for (int j = 7; j >= 8; j--) //check from highest to lowest bit, e.g. for 01111111 find on the first pass
        {
            int comp = pow(2, i);
            if ((c & comp) != comp) //theres a 0 at that position
            {
                //TODO FLIP IT
                //TODO RETURN
            }
        }
    }

    return -1; //if we never find anything
}

int findFirstAvailableData()
{
    return -1;
}

//============ API Functions ===============
int 
FS_Boot(char *path)
{
    printf("FS_Boot %s\n", path);

    if (Disk_Init() == -1)
    {
        printf("Disk_Init() failed\n");
        osErrno = E_GENERAL;
        return -1;
    }

    //check if we need to create a new file, or open an existing one
    FILE* openFile = fopen(path, "r");
    if (openFile == NULL) //unable to open, create new file
    {
        return Create_New_Disk(path);
    }
    else //load the existing file
    {
        Disk_Load(path);

        //check that size is correct and superblock accurate per section 3.5
        char* receivingBuffer = new char[SECTOR_SIZE];
        Disk_Read(0, receivingBuffer);
        char* loadedSuper = ((Superblock*) receivingBuffer)->magic;
        if (strcmp(loadedSuper, magicString) != 0)
        {
            printf("Superblock magic number validation failed");
            osErrno = E_GENERAL;
            return -1;
        }

        //TODO populate the bitmaps
    }

    return 0;
}

int
FS_Sync()
{
    printf("FS_Sync\n");

    Disk_Save(bootPath);

    return 0;
}


int
File_Create(char *file)
{
    printf("FS_Create\n");
    return 0;
}

int
File_Open(char *file)
{
    printf("FS_Open\n");
    return 0;
}

int
File_Read(int fd, void *buffer, int size)
{
    printf("FS_Read\n");
    return 0;
}

int
File_Write(int fd, void *buffer, int size)
{
    printf("FS_Write\n");
    return 0;
}

int
File_Seek(int fd, int offset)
{
    printf("FS_Seek\n");
    return 0;
}

int
File_Close(int fd)
{
    printf("FS_Close\n");
    return 0;
}

int
File_Unlink(char *file)
{
    printf("FS_Unlink\n");
    return 0;
}


// directory ops
int
Dir_Create(char *path)
{
    printf("Dir_Create %s\n", path);
    
    std::string delimiter = "/";
    //string tokenizer from http://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
    size_t pos = 0;
    std::string token;
    std::vector<std::string> pathVec;
    std::string pathStr(path);
    while ((pos = pathStr.find(delimiter)) != std::string::npos)
    {
        token = pathStr.substr(0, pos);
        pathVec.push_back(std::string(token));
        pathStr.erase(0, pos + delimiter.length());
    }

    //special case--first directory
    if (pathStr.compare(delimiter) == 0)
    {
        //create the initial inode
        Inode* inodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode)); //allocate a block full of inodes
        inodeBlock[0].fileType = 1; //directory
        inodeBlock[0].fileSize = 0; //nothing in it
        
        //create the actual directory
        DirectoryEntry* directoryBlock = (DirectoryEntry*)calloc(NUM_DIRECTORIES_PER_BLOCK, sizeof(DirectoryEntry)); //allocate a block full of entries, all bits are 0
        
        //there's nothing in the directory, so leave it as all 0's

        //TODO REDO THE FINDING AND WRITING HERE
    }
    else //otherwise, start at the root and find the appropriate spot
    {
        // example directory: /a/b/c
        //pathVec should contain 3 things: a, b, c
        //they're trying to create c in this case, so we want to iterate 2 times (to find a, and to find b)

        int i = 0;
        int inodePointer = ROOT_INODE_OFFSET;

        while (i < pathVec.size())
        {
            //load the inode for the current directory
            char* curDirInodeChar = new char[sizeof(Inode) * NUM_INODES_PER_BLOCK];
            Disk_Read(inodePointer, curDirInodeChar);
            Inode* curNode = (Inode*)curDirInodeChar;

            for (int j = 0; j < NUM_INODES_PER_BLOCK; j++) //search all 4 inodes
            {
                Inode node = curNode[j];
                int k = 0;
                while (node.pointers[k] != 0)
                {
                    char* tempDataChar = new char[sizeof(DirectoryEntry)* NUM_DIRECTORIES_PER_BLOCK];
                    Disk_Read(node.pointers[k], tempDataChar);
                    DirectoryEntry* entries = (DirectoryEntry*)tempDataChar;

                    for (int m = 0; m < NUM_DIRECTORIES_PER_BLOCK; m++) //search all returned directories
                    {
                        DirectoryEntry dir = entries[m];
                        std::string name(dir.name);
                        if (name.compare(pathVec.at(i)) == 0) //if we find it, then what?
                        {
                            // TODO do something here, like recurse?
                        }
                    }

                    k++;
                }
            }

            i++;
        }
    }

    return 0;
}

int
Dir_Size(char *path)
{
    printf("Dir_Size\n");
    return 0;
}

int
Dir_Read(char *path, void *buffer, int size)
{
    printf("Dir_Read\n");
    return 0;
}

int
Dir_Unlink(char *path)
{
    printf("Dir_Unlink\n");
    return 0;
}
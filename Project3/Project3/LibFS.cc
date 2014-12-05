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

//Searches the given inode for the current pathsegment
//Recursive
int searchInodeForPath(int inodeToSearch, std::vector<std::string>& path, int pathSegment)
{
    //Base case 1: If we're at the end of the path, we're already in the target node
    //Just return!
    if (pathSegment + 1 == path.size())
    {
        return inodeToSearch;
    }

    //First, load the current directory inode
    Inode* inodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
    Disk_Read((inodeToSearch / NUM_INODES_PER_BLOCK), (char*)inodeBlock); //floor divide by 4 to get the actual sector, e.g. if pass inode 6 we want block 1
    Inode curNode = inodeBlock[inodeToSearch % NUM_INODES_PER_BLOCK]; //mod to get the actual inode

    //Search the contents of the current inode
    int i = curNode.pointers[0];
    while (i != 0)
    {
        DirectoryEntry* directoryBlock = (DirectoryEntry*)calloc(NUM_DIRECTORIES_PER_BLOCK, sizeof(DirectoryEntry));
        Disk_Read(curNode.pointers[i], (char*)directoryBlock);
        for (int j = 0; j < NUM_DIRECTORIES_PER_BLOCK; j++)
        {
            DirectoryEntry curEntry = directoryBlock[j];
            std::string name(curEntry.name);
            if (name.compare(path.at(pathSegment))) //if we find the next directory, recurse into it
                //TODO this assumes it's actually a directory. we could be trying to create a directory c when there's already a file called c
                //I think we're going to have to pull the actual inode and check the type
                //this might be a fairly minor case--come back to it
            {
                return searchInodeForPath(curEntry.inodeNum, path, pathSegment++);
            }
            //else continue
        }
        i++;
    }

    return -1;
    //TODO error case base case 2
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
File_Close(int fd)
{
    printf("FS_Close\n");
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

    //create the inode
    Inode* inodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode)); //allocate a block full of inodes
    inodeBlock[0].fileType = 1; //directory
    inodeBlock[0].fileSize = 0; //nothing in it

    //create the actual directory
    DirectoryEntry* directoryBlock = (DirectoryEntry*)calloc(NUM_DIRECTORIES_PER_BLOCK, sizeof(DirectoryEntry)); //allocate a block full of entries, all bits are 0
    int directorySector = (findFirstAvailableData() / NUM_INODES_PER_BLOCK); //again, floor by 4 since it might give us inode 9, which is the start of block 2...

    inodeBlock[0].pointers[0] = directorySector;

    //special case--first directory
    if (pathStr.compare(delimiter) == 0)
    {
        //there's nothing in the directory, so leave it as all 0's

        inodeBlock[0].pointers[0] = directorySector;

        Disk_Write(INODE_BITMAP_OFFSET, (char*)inodeBlock);
        Disk_Write(directorySector, (char*)directoryBlock);
    }
    else //otherwise, start at the root and find the appropriate spot
    {
        int newInodeNum = findFirstAvailableInode();

        int parentInodeNum = searchInodeForPath(ROOT_INODE_OFFSET, pathVec, 0);
        //in a path /a/b/c, trying to add c, parentInode gives us the inode for b

        Inode* parentInodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
        Disk_Read((parentInodeNum / 4), (char*)parentInodeBlock);

        //Now, insert a directoryentry for c into the directory block pointed to by b's inode
        bool inserted = false;
        int i = 0;
        while (!inserted) //check the parent inodes pointers
        {
            if (i == 30)
            {
                //TODO error case
                //we've checked all pointers at this point and not made an insertion
                return -1;
            }

            int entrySector = parentInodeBlock[parentInodeNum % NUM_INODES_PER_BLOCK].pointers[i];
            if (entrySector == 0)
            {
                //if we've hit a pointer to 0, we need a new Directory sector and everything. find a new one with the bitmap and create it normally.
                DirectoryEntry* newEntry = (DirectoryEntry*)calloc(NUM_DIRECTORIES_PER_BLOCK, sizeof(DirectoryEntry));
                newEntry[0].inodeNum = newInodeNum;
                strcpy(newEntry[0].name, pathVec.at(pathVec.size() - 1).c_str());
                int newDirectorySector = findFirstAvailableData();
                Disk_Write(newDirectorySector, (char*)newEntry);
                parentInodeBlock[parentInodeNum % NUM_INODES_PER_BLOCK].pointers[i] = newDirectorySector;

                inserted = true;
                break;
            }

            DirectoryEntry* entryBlock = (DirectoryEntry*)calloc(NUM_DIRECTORIES_PER_BLOCK, sizeof(DirectoryEntry));
            Disk_Read(entrySector, (char*)entryBlock);

            for (int j = 0; j < NUM_DIRECTORIES_PER_BLOCK; j++)
            {
                if (entryBlock[j].inodeNum == 0) //can't possibly be the superblock! calloc should set it to 0 initially
                {
                    strcpy(entryBlock[j].name, pathVec.at(pathVec.size() - 1).c_str()); //copy the end of the path name into the new entry
                    entryBlock[j].inodeNum = newInodeNum;
                    Disk_Write(entrySector, (char*)entryBlock); //update the entry
                    inserted = true;
                }
            }

            i++;
        }
    }

    return 0;
}
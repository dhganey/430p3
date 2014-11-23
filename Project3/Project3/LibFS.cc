#include "LibFS.h"
#include "LibDisk.h"

#include<string>

// global errno value here
int osErrno;

//const resources
char* const magicString = "666";
const int numInodes = 250;
const int numDataBlocks = 9744; // TODO COME BACK TO THIS

//sector "pointer" offsets
const int superblockOffset = 0;
const int inodeOffset = 1;
const int dataOffset = 2;

//bitmaps
char* inodeBitmap;
char* dataBitmap;

//structs
typedef struct dataBlock
{
    char data[SECTOR_SIZE];
} DataBlock;

//============ Helper Functions ============
int
Create_New_Disk(char* path)
{
    int ok = 0;

    //prep the superblock
    //superblock contains "666" followed by garbage
    char* superString = new char[SECTOR_SIZE];
    superString[0] = '6';
    superString[1] = '6';
    superString[2] = '6';

    ok = Disk_Write(superblockOffset, superString);
    if (ok == -1)
    {
        osErrno = E_CREATE;
        return ok;
    }
    
    //prep the bitmaps
    inodeBitmap = new char[numInodes];
    for (int i = 0; i < numInodes; i++)
    {
        inodeBitmap[i] = 0;
    }
    dataBitmap = new char[numDataBlocks];
    for (int i = 0; i < numDataBlocks; i++)
    {
        dataBitmap[i] = 0;
    }

    //put the bitmaps on the disk
    ok = File_Write(inodeOffset, inodeBitmap, sizeof(inodeBitmap));
    if (ok == -1)
    {
        osErrno = E_CREATE;
        return ok;
    }
    ok = File_Write(dataOffset, dataBitmap, sizeof(dataBitmap));
    if (ok == -1)
    {
        osErrno = E_CREATE;
        return ok;
    }

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
        char* receivingBuffer = new char[3];
        Disk_Read(0, receivingBuffer);
        if (strcmp(receivingBuffer, magicString) != 0)
        {
            printf("Superblock magic number validation failed");
            osErrno = E_GENERAL;
            return -1;
        }

        //retrieve the bitmaps from disk
        Disk_Read(inodeOffset, inodeBitmap);
        Disk_Read(dataOffset, dataBitmap); //TODO THIS WILL NOT WORK, THIS READS ONLY THE FIRST SECTOR
    }

    return 0;
}

int
FS_Sync()
{
    printf("FS_Sync\n");
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
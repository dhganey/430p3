#include "LibFS.h"
#include "LibDisk.h"

#include <string>
#include <vector>
#include <iostream>
#include <math.h>
#include <unordered_map>

#define DEBUG true

// global errno value here
int osErrno;

int totalFilesAndDirectories = 0;
int fileDescriptorCount = 0;
char* magicString = "666";
char* bootPath; //we'll populate this after boot so we can call sync

//structs

typedef struct superblock
{
    char magic[4]; //one extra for \0
    char garbage[SECTOR_SIZE - 3];
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

typedef struct filedata
{
    char contents[SECTOR_SIZE];
} FileData;

typedef struct openfile
{
    int inodeNum;
    int filepointer;
    char garbage[SECTOR_SIZE - 2 * sizeof(int)];
} OpenFile;

//Maps from file descriptors to open file structs
typedef std::unordered_map<int, OpenFile> OpenFileMap;

OpenFileMap openFileTable;

Bitmap* inodeBitmap;
Bitmap* dataBitmap;

//============ Helper Functions ============
int
Create_New_Disk(char* path)
{
    int ok = 0;

    //prep the superblock
    Superblock* super = (Superblock*)calloc(1, sizeof(Superblock));
    strcpy(super->magic, magicString);
    ok = Disk_Write(SUPER_BLOCK_OFFSET, (char*)super);

    if (ok == -1)
    {
        osErrno = E_CREATE;
        return ok;
    }
    
    inodeBitmap = (Bitmap*)calloc(1, sizeof(Bitmap));
    dataBitmap = (Bitmap*)calloc(1, sizeof(Bitmap));

    Disk_Write(INODE_BITMAP_OFFSET, (char*)inodeBitmap);
    Disk_Write(DATA_BITMAP_OFFSET, (char*)dataBitmap);

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
//Returns the inode number, NOT the sector number! Must be adjusted by the caller before writing
//This is because we need the actual inode number for pointers
int findFirstAvailableInode()
{
    //Look through each char
    for (int i = 0; i < NUM_CHARS; i++)
    {
        char c = inodeBitmap->bits[i];
        //Each char represents 8 bits
        for (int j = 7; j >= 0; j--) //check from highest to lowest bit, e.g. for 01111111 find on the first pass
        {
            int comp = pow(2, j);
            if ((c & comp) != comp) //theres a 0 at that position
            {
                //we want to flip the jth bit
                c |= 1 << j; //this should do it: http://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit-in-c-c
                inodeBitmap->bits[i] = c;

                //now return the inode num
                return ((i * 8) + abs(j - 7)); //TODO this might not be right
            }
        }
    }

    return -1; //if we never find anything
}

//Finds the first 0 in the data bitmap
//Returns the SECTOR number since data is considered in whole sectors rather than pieces
int findFirstAvailableDataSector()
{
    //Look through each char
    for (int i = 0; i < NUM_CHARS; i++)
    {
        char c = dataBitmap->bits[i];
        //Each char represents 8 bits
        for (int j = 7; j >= 0; j--)
        {
            int comp = pow(2, j);
            if ((c & comp) != comp) //0 in that position
            {
                c |= 1 << j;
                dataBitmap->bits[i] = c;

                //return the sector number.
                //still the same return function as above! here, each bit maps to an entire block
                return ((i * 8) + abs(j - 7)) + FIRST_DATABLOCK_OFFSET; //TODO this might not be right
            }
        }
    }
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
    int inodeSector = (inodeToSearch / NUM_INODES_PER_BLOCK) + ROOT_INODE_OFFSET;
    Disk_Read(inodeSector, (char*)inodeBlock);
    Inode curNode = inodeBlock[inodeToSearch % NUM_INODES_PER_BLOCK]; //mod to get the actual inode

    //Search the contents of the current inode
    int i = 0;
    while (curNode.pointers[i] != 0)
    {
        DirectoryEntry* directoryBlock = (DirectoryEntry*)calloc(NUM_DIRECTORIES_PER_BLOCK, sizeof(DirectoryEntry));
        Disk_Read(curNode.pointers[i], (char*)directoryBlock);
        for (int j = 0; j < NUM_DIRECTORIES_PER_BLOCK; j++)
        {
            DirectoryEntry curEntry = directoryBlock[j];
            std::string name(curEntry.name);
            if (name.compare(path.at(pathSegment)) == 0) //if we find the next directory, recurse into it
                //TODO this assumes it's actually a directory. but this might be recursing into a file, and that's going to cause serious problems
            {
                return searchInodeForPath(curEntry.inodeNum, path, pathSegment + 1);
            }
            //else continue
        }
        i++;
    }

    return -1;
    //TODO error case base case 2
}

std::vector<std::string> tokenizePathToVector(std::string pathStr)
{
    std::string delimiter = "/";
    //string tokenizer from http://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
    size_t pos = 0;
    std::string token;
    std::vector<std::string> pathVec;
    while ((pos = pathStr.find(delimiter)) != std::string::npos)
    {
        token = pathStr.substr(0, pos);
        if (std::string(token).compare("") != 0)
        {
            pathVec.push_back(std::string(token));
        }
        pathStr.erase(0, pos + delimiter.length());
    }

    pathVec.push_back(pathStr); //tokenizer skips the last entry
    return pathVec;
}

int insertDirectoryEntry(std::vector<std::string>& pathVec, int parentInodeNum, int newInodeNum)
{
    Inode* parentInodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
    int parentInodeSector = (parentInodeNum / NUM_INODES_PER_BLOCK) + ROOT_INODE_OFFSET;
    Disk_Read(parentInodeSector, (char*)parentInodeBlock);

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
            int newDirectorySector = findFirstAvailableDataSector();
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
                break;
            }
        }

        i++;
    }

    return 0;
}

//============ API Functions ===============
int FS_Boot(char *path)
{
    printf("FS_Boot %s\n", path);
    bootPath = path;

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
        Superblock* super = (Superblock*)calloc(1, sizeof(Superblock));
        Disk_Read(SUPER_BLOCK_OFFSET, (char*)super);
        if (strcmp(super->magic, magicString) != 0)
        {
            printf("Superblock magic number validation failed");
            std::cout << "Actually found " << super->magic << std::endl;
            osErrno = E_GENERAL;
            return -1;
        }

        inodeBitmap = (Bitmap*)calloc(1, sizeof(Bitmap));
        dataBitmap = (Bitmap*)calloc(1, sizeof(Bitmap));

        Disk_Read(INODE_BITMAP_OFFSET, (char*)inodeBitmap);
        Disk_Read(DATA_BITMAP_OFFSET, (char*)dataBitmap);
    }

    return 0;
}

int FS_Sync()
{
    printf("FS_Sync\n");

    Disk_Save(bootPath);

    return 0;
}


int File_Create(char *file)
{
    printf("FS_Create\n");

    if (DEBUG)
    {
        std::cout << "Before creating file, verify the root node:" << std::endl;
        Inode* rootNode = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
        Disk_Read(ROOT_INODE_OFFSET, (char*)rootNode);
        Inode node = rootNode[0];
        std::cout << "Root node has filetype " << node.fileType << " and pointers:" << std::endl;
        for (int i = 0; i < 30; i++)
        {
            std::cout << node.pointers[i] << " ";
        }
        std::cout << std::endl;
    }

    std::string pathStr(file);
    std::vector <std::string> pathVec = tokenizePathToVector(pathStr);

    int newInodeNum = findFirstAvailableInode();
    int newInodeSector = newInodeNum / NUM_INODES_PER_BLOCK + ROOT_INODE_OFFSET; //TODO: APPLY THIS FIX EVERYWHERE WE READ. this is the clear way of doing it (compute a sector outside of the read call)

    int parentInodeNum = searchInodeForPath(0, pathVec, 0);
    insertDirectoryEntry(pathVec, parentInodeNum, newInodeNum);

    //now create the new inode for the file
    Inode* newNodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
    Disk_Read(newInodeSector, (char*)newNodeBlock);
    newNodeBlock[newInodeNum % NUM_INODES_PER_BLOCK].fileType = 0;
    newNodeBlock[newInodeNum % NUM_INODES_PER_BLOCK].fileSize = 0;
    //that's it, I think! No need to point to anything since they've not tried to write yet

    Disk_Write(newInodeSector, (char*)newNodeBlock);

    totalFilesAndDirectories++;

    return 0;
}

//Returns a fd, file descriptor
int File_Open(char *file)
{
    printf("FS_Open\n");

    std::string pathStr(file);
    std::vector<std::string> pathVec = tokenizePathToVector(pathStr);

    //Grab the parent inode
    int parentInodeNum = searchInodeForPath(0, pathVec, 0);
    Inode* parentInodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
    int inodeSector = (parentInodeNum / NUM_INODES_PER_BLOCK) + ROOT_INODE_OFFSET;
    Disk_Read(inodeSector, (char*)parentInodeBlock);
    Inode parentInode = parentInodeBlock[parentInodeNum % NUM_INODES_PER_BLOCK];

    if (parentInode.fileType != 1) //if not a directory
    {
        return -1; //TODO error case
    }

    //Now that we have the parent inode, search the contents of its directory
    //for the file we're trying to open
    for (int i = 0; i < 30; i++) //go through all the pointers
    {
        DirectoryEntry* dirBlock = (DirectoryEntry*)calloc(NUM_DIRECTORIES_PER_BLOCK, sizeof(DirectoryEntry));
        Disk_Read(parentInode.pointers[i], (char*)dirBlock);
        for (int j = 0; j < NUM_DIRECTORIES_PER_BLOCK; j++) //go through all the directories for the given pointer
        {
            DirectoryEntry curEntry = dirBlock[j];
            if (strcmp(curEntry.name, pathVec.at(pathVec.size() - 1).c_str()) == 0) //if we find the file, open it!
            {
                //we need the size of the file
                //grab that inodenum
                Inode* nodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
                int inodeSector = (curEntry.inodeNum / NUM_INODES_PER_BLOCK) + ROOT_INODE_OFFSET;
                Disk_Read(inodeSector, (char*)nodeBlock);
                Inode curNode = nodeBlock[curEntry.inodeNum % NUM_INODES_PER_BLOCK];

                OpenFile of;
                of.filepointer = curNode.fileSize;
                of.inodeNum = curEntry.inodeNum;
                openFileTable.insert(std::pair<int, OpenFile>(fileDescriptorCount, of));
                fileDescriptorCount++; //increase for uniqueness, BUT:
                return (fileDescriptorCount - 1); //return the one we saved!

                //TODO: again, note that this returns the inode for anything with the right name
                //this could be opening a directory, i think
            }
        }
    }

    return 0;
}

int File_Write(int fd, void *buffer, int size)
{
    printf("FS_Write\n");
    OpenFileMap::iterator it = openFileTable.find(fd);
    if (it == openFileTable.end())
    {
        return -1;
        //TODO error case
    }

    OpenFile open = openFileTable.at(fd);
    //get the inode of the file
    Inode* inodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
    int inodeSector = (open.inodeNum / NUM_INODES_PER_BLOCK) + ROOT_INODE_OFFSET;
    Disk_Read(inodeSector, (char*)inodeBlock);
    Inode curNode = inodeBlock[open.inodeNum % NUM_INODES_PER_BLOCK];

    int filePointer = open.filepointer;
    
    if (filePointer + size > 30 * SECTOR_SIZE)
    {
        return -1;
        //TODO error case, too big?
    }

    int remainingSize = size;
    while (remainingSize > 0)
    {
        int filePointerForBlock = filePointer % SECTOR_SIZE;
        FileData* writeBlock;
        int dataSector;

        if (filePointerForBlock == 0) //at the beginning, create a new one
        {
            writeBlock = new FileData();
            dataSector = findFirstAvailableDataSector();
            inodeBlock[open.inodeNum % NUM_INODES_PER_BLOCK].pointers[filePointer / SECTOR_SIZE] = dataSector;
            Disk_Write(inodeSector, (char*)inodeBlock);
        }
        else
        {
            dataSector = curNode.pointers[filePointer / SECTOR_SIZE];
            Disk_Read(dataSector, (char*)writeBlock);
        }

        int remainingSizeBackup = remainingSize;
        for (int i = filePointerForBlock; i < remainingSizeBackup && filePointerForBlock < SECTOR_SIZE; filePointerForBlock++, filePointer++, remainingSize--)
        {
            writeBlock->contents[i] = ((char*)buffer)[i];
        }

        Disk_Write(dataSector, (char*)writeBlock);
        filePointer++;
    }
    return 0;
}

int File_Close(int fd)
{
    printf("FS_Close\n");

    OpenFileMap::iterator it = openFileTable.find(fd);
    if (it == openFileTable.end())
    {
        return -1;
        //TODO ERROR CASE
    }

    //if we found it, close the file and get out of here
    openFileTable.erase(fd);
    return 0;
}


// directory ops
int Dir_Create(char *path)
{
    printf("Dir_Create %s\n", path);
    std::string pathStr(path);
    std::vector<std::string> pathVec = tokenizePathToVector(pathStr);

    //create the inode
    Inode* inodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode)); //allocate a block full of inodes

    //create the actual directory
    DirectoryEntry* directoryBlock = (DirectoryEntry*)calloc(NUM_DIRECTORIES_PER_BLOCK, sizeof(DirectoryEntry)); //allocate a block full of entries, all bits are 0
    int directorySector = findFirstAvailableDataSector(); //note that this does not have to be floor divided, it returns the SECTOR

    //special case--first directory
    if (pathStr.compare("/") == 0)
    {
        //there's nothing in the directory, so leave it as all 0's

        inodeBlock[0].fileType = 1; //directory
        inodeBlock[0].fileSize = 0; //nothing in it
        inodeBlock[0].pointers[0] = directorySector;

        findFirstAvailableInode(); //TODO added this because i guess we're not reporting the root as filled

        Disk_Write(ROOT_INODE_OFFSET, (char*)inodeBlock);
        Disk_Write(directorySector, (char*)directoryBlock);
        if (DEBUG)
        {
            std::cout << "Created root directory " << std::endl;
        }
    }
    else //otherwise, start at the root and find the appropriate spot
    {
        int newInodeNum = findFirstAvailableInode();
        int newInodeSector = (newInodeNum / NUM_INODES_PER_BLOCK) + ROOT_INODE_OFFSET;
        Disk_Read(newInodeSector, (char*)inodeBlock);
        inodeBlock[newInodeNum % NUM_INODES_PER_BLOCK].fileType = 1; //update the appropriate part of the inode block
        inodeBlock[newInodeNum % NUM_INODES_PER_BLOCK].fileSize = 0;
        inodeBlock[newInodeNum % NUM_INODES_PER_BLOCK].pointers[0] = directorySector;

        int parentInodeNum = searchInodeForPath(0, pathVec, 0);
        //in a path /a/b/c, trying to add c, parentInode gives us the inode for b

        insertDirectoryEntry(pathVec, parentInodeNum, newInodeNum);

        //By this point, a directory entry for c has been entered into b's directory record
        //All that's left to do is write the inode and directory entry for the new directory
        if (DEBUG)
        {
            std::cout << "Creating dir. Writing inode block containing:" << std::endl;
            for (int i = 0; i < NUM_INODES_PER_BLOCK; i++)
            {
                std::cout << "Inode " << i << " filetype " << inodeBlock[i].fileType << " with pointers: " << std::endl;
                for (int j = 0; j < 30; j++)
                {
                    std::cout << inodeBlock[i].pointers[j] << " ";
                }
                std::cout << std::endl;
            }
        }
        Disk_Write(newInodeSector, (char*)inodeBlock);
        Disk_Write(directorySector, (char*)directoryBlock);
    }

    totalFilesAndDirectories++;
    return 0;
}






void validateRoot()
{
    Inode* rootBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));
    Disk_Read(ROOT_INODE_OFFSET, (char*)rootBlock);
    Inode rootNode = rootBlock[0];
    std::cout << "Root block filetype " << rootNode.fileType << " with pointers:\n";
    for (int i = 0; i < 30; i++)
    {
        std::cout << rootNode.pointers[i] << " ";
    }
    std::cout << std::endl;
}

void printInodes()
{
    Inode* inodeBlock = (Inode*)calloc(NUM_INODES_PER_BLOCK, sizeof(Inode));

    for (int i = ROOT_INODE_OFFSET; i < FIRST_DATABLOCK_OFFSET; i++)
    {
        Disk_Read(i, (char*)inodeBlock);
        for (int j = 0; j < NUM_INODES_PER_BLOCK; j++)
        {
            Inode curNode = inodeBlock[j];
            if (curNode.fileType == 1)
            {
                std::cout << "Directory at inode #" << i << " with pointers:\n";
                for (int k = 0; k < 30; k++)
                {
                    std::cout << curNode.pointers[k] << " ";
                }
                std::cout << std::endl;
            }
        }
    }
}
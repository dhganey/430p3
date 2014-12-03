#include "LibFS.h"
#include "LibDisk.h"

#include <string>
#include <vector>
#include <bitset>
#include <iostream>

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
    
    ok = Disk_Write(INODE_BITMAP_OFFSET, convertBitsetToChar(inodeBitmap));
    ok = Disk_Write(DATA_BITMAP_OFFSET, convertBitsetToChar(dataBitmap)); //TODO this must be able to write across multiple sections!

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
//Changes it to a 1 and returns the index in the bitmap
int findAndFillAvailableInodeBlock()
{
    for (int i = 0; i < inodeBitmap.size(); i++)
    {
        if (!inodeBitmap.test(i))
        {
            inodeBitmap.flip(i);
            return i;
        }
    }

    return -1;
}

//Finds the first 0 in the data bitmap
//Changes it to a 1 and returns the index in the bitmap
int findAndFillAvailableDataBlock()
{
    for (int i = 0; i < dataBitmap.size(); i++)
    {
        if (!dataBitmap.test(i))
        {
            dataBitmap.flip(i);
            return i;
        }
    }

    return -1;
}

//Given a bitset, convert it to a char*
char* convertBitsetToChar(std::bitset<NUM_INODES> set)
{
    std::vector<char> charVec;
    std::string bitString(set.to_string());

    for (int i = 0; i < NUM_CHARS; i++) //there are 125 chars, each representing 8 bits
    {
        int x = i * 8; //starting position
        std::string subStr = bitString.substr(x, 8);
        char c = strtol(subStr.c_str(), NULL, 2);
        charVec.push_back(c);
    }
    
    char* chars = new char[NUM_CHARS];
    std::copy(charVec.begin(), charVec.end(), chars);
    return chars;
}

//Given a c string, convert it to a bitset
std::bitset<NUM_INODES> convertCharToBitset(char* str)
{
    std::bitset<NUM_INODES> retSet;
    
    for (int i = 0; i < NUM_CHARS; i++) //assume the string has the right num chars
    {
        //convert each char to binary
        int x = str[i];
        char buffer[8];
        itoa(x, buffer, 2); //base 2

        std::string bitStr(buffer);
        for (int j = 0; j < 8; j++)
        {
            if (bitStr.at(j) == '1')
            {
                retSet.flip(i * 8 + j);
            }
        }
    }

    return retSet;
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

        //read in the bitmaps
        char* tempInode = new char[NUM_CHARS];
        char* tempData = new char[NUM_CHARS];

        Disk_Read(INODE_BITMAP_OFFSET, tempInode);
        Disk_Read(DATA_BITMAP_OFFSET, tempData);

        inodeBitmap = convertCharToBitset(tempInode);
        dataBitmap = convertCharToBitset(tempData);
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

int main()
{
    Disk_Init();

    std::bitset<1000> setName;
    setName.flip(2);
    setName.flip(3);
    setName.flip(7);

    std::cout << setName.to_string();
    std::cout << "\n\n\n";

    char* chars = new char[125];
    chars = convertBitsetToChar(setName);
    std::bitset<1000> newSet = convertCharToBitset(chars);

    std::cout << newSet.to_string();
}
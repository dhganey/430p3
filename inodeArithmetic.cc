int inodeNum; //either a parameter or returned result of bitmap function.
Inode* inodeBlock = (Inode*)calloc(4, sizeof(Inode)); //allocate an entire block of Inodes
//to modify a particular inode:
inodeBlock[inodeNum % 4].fileType = 1;
//to save the whole block:
int inodeSector = inodeNum / 4 + ROOT_INODE_OFFSET;
Disk_Write(inodeSector, (char*)inodeBlock);
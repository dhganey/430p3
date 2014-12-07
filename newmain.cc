#include <iostream>

#include "LibDisk.h"
#include "LibFS.h"

int main(int argc, char* argv[])
{
    char* imagePath = argv[1];

    FS_Boot(imagePath);

    int ok;
    ok = Dir_Create("/dir1");
    ok = Dir_Create("/dir1/sub1");
    ok = Dir_Create("/dir2");
    ok = Dir_Create("/dir2/sub2");

    ok = Dir_Create("/dir1/sub1");
    if (ok == -1)
    {
        std::cout << "Correctly prevented duplicate /dir1/sub1" << std::endl;
    }

    ok = File_Create("/dir1/sub1");
    if (ok == -1)
    {
        std::cout << "Correctly prevent duplicate /dir1/sub1 FILE" << std::endl;
    }

    ok = File_Create("/dir1/sub.txt");
    ok = Dir_Create("/dir1/sub.txt");
    if (ok == -1)
    {
        std::cout << "Correctly prevent duplicate /dir1/sub.txt DIRECTORY" << std::endl;
    }
}
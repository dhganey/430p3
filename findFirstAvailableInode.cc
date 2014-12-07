int findFirstAvailableInode()
{
    for (int i = 0; i < NUM_CHARS; i++)
    {
        char c = inodeBitmap->bits[i];
        for (int j = 7; j >= 0; j--) //check from highest to lowest bit
        {
            int comp = pow(2, j);
            if ((c & comp) != comp) //theres a 0 at that position
            {
                //we want to flip the jth bit
                c |= 1 << j;
                inodeBitmap->bits[i] = c;

                //now return the inode num
                return ((i * 8) + abs(j - 7));
            }
        }
    }

    return -1; //if we never find anything
}

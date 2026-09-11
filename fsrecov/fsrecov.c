/*

Lab fsrecov :
In this lab, we will implement a cli tool that can recover img files from a mkfs.vfat 
filesystem. The most file in the filesystem are BMP files, we try to recover most of them.

The command :
fsrecov <File> , (File is a Fat32 image file)

When you recover a BMP file(the header and all the data), you should get its SHA1 fingerprint with sha1sum and 
print it and BMP file's name to the STDOUT. Only when the SHA1 fingerprint and name are right, we think the 
BMP file is recovered successfully.

Sector : The least size of physical storage unit. When disk read or write, at least a sector ...
Block: The least size of logical storage unit, when you use read() or write() , at least a block ...
Cluster: The least size of file storage unit, when you alloc a file, at least a cluster ...

We can use mkfs.fat to create a FAT32 image file for testing, for example:
mkfs.fat -v -F 32 -S 512 -s 8 fs.img (which force the block size is 1024 bytes)

so our fs.img has 64 MB / 1024 bytes = 65536 blocks
one cluster has 8 sectors, so one cluster has 8 * 512 = 4 KB, which is the minimum size of a file in this filesystem.

When we get our fs.img , we can write some BMP files to it or delete some BMP files from it .
And at last, we use mkfs.fat -v -F 32 -S 512 -s 8 fs.img again to format the fs.img .
Now the fs.img is the target that we want to recover BMP files from it.

Clusters in data area :
1. Directory file, which stores many directory entries, please read Section 6 and 7 of the FAT32 sepcification..
2. BMP header file, which starts with 0x42 0x4d (BM)
3. BMP data file, which 3 bytes per pixel
4. unused cluster ...

Oh my god , please read the "friendly" material of FAT specification.cd
Ok, I finish reading the FAT specification !!! It is hard and sticky, but we can do it .



*/


int main(int argc, char *argv[]) {
}

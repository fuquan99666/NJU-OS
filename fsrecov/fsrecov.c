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

Steps:
1. To read the FAT32 image file, we can use mmap to map the image file to memory, and then we can read the FAT32 image file from memory.
2. Then parse the first sector of the image file, which contains BPB info ...
3. Then we can scan the data clusters from BPB_RootClus (ofter is 2) to last cluster, and divide them to :
    1. Directory file, which stores many BMP files entry (you can know this because many ".bmp" bytes in it)
    2. BMP header file, which starts with 0x42 0x4d (BM)
    3. BMP data file, which 3 bytes per pixel 
    4. unused cluster, which is all 0x00 bytes

4. Then we can get each bmp file's first cluster number and its name from the entry ...
   Here is a problem that because of we lost our FAT table, so we don't know the next cluster number except the first one, 
   so we can only assume that the file is continuous (bushi) and we can get the entire bmp file based on its size (which can be got from the entry)

5. After we get some bmp files (its name, size and data), we can use sha1sum to get its SHA1 fingerprint and print it to the STDOUT
   Maybe we can compare the SHA1 fingerprint with the original one to check if we recover the bmp file successfully 
   Or you can just save the bmp file to /tmp and look at it to check if it is right (bushi)


*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "fat32.h"

struct fat32hdr *bpb = NULL;
void *mapped_image = NULL;

// we pass in the current cluster's sector number
// and check if it is a directory file which contains many bmp files entry 
int check_directory_file(u32 current_cluster) {
    
    // how can we include the current_cluster is a directory file ?
    // Well, in this lab, we assume that the image contains many bmp files, so if 
    // the current cluster is a directory file, it should contains many "BMP" bytes 
    // our code is based on this assumption ...

    // if it is a directory file, each entry of it should be 32 bytes which containes
    // the name of the "xxx.bmp" , we can check the first sector of the current cluster 
    // count the number of "BMP" bytes, if the count is more than 1, we can assume that it is a directory file 

    int bmp_count = 0;

    // firstly get the address of the current cluster's first sector 
    char *sector_addr = (char *) mapped_image + current_cluster * bpb->BPB_BytsPerSec;

    // read the first sector of sector_addr, which is 512 bytes, and count the number of "BMP" bytes 
    // it may be a short entry or a long entries ...
    for (int i = 0; i < 512; i+=32) {
        // we just check the short entry, the "BMP" should be in the 8-10 bytes of the entry 
        char *entry = sector_addr + i;
        if (entry[8] == 'B' && entry[9] == 'M' && entry[10] == 'P') {
            bmp_count++;
        }
    }

    if (bmp_count > 1) {
        // contains many "BMP" bytes, it must be a directory file 
        return 1;
    }

    return 0;
}

void get_bmp(u32 first_cluster, u32 bmp_size, char *name) {
    // if the size of the bmp file is larger than a cluster, we can assume that 
    // the bmp file is continuous 

    // firstly get the address of the first cluster's address 
    char *cluster_addr = (char *) mapped_image + (bpb->BPB_RsvdSecCnt + bpb->BPB_NumFATs * bpb->BPB_FATSz32 + (first_cluster - 2) * bpb->BPB_SecPerClus) * bpb->BPB_BytsPerSec;

    // then we can read the bpm file's data from the cluster_addr
    // we can firstly get the magic number of the bmp file : 424d !
    u16 magic = *(u16 *) cluster_addr;
    if (magic != 0x4d42) {
        fprintf(stderr, "Error: %s is not a bmp file, its magic number is wrong !\n", name);
        return;
    }

    // then maybe we can check its size ?
    u32 size = *(u32 *) (cluster_addr + 2);
    if (size != bmp_size) {
        fprintf(stderr, "Warning: %s's size is not equal to the size in the entry ...\n", name);
        return ;
    }

    // then maybe we can save the bmp file to /tmp directory 
    char path[256];
    snprintf(path, sizeof(path), "/tmp/%s", name);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    if (write(fd, cluster_addr, bmp_size) == -1) {
        perror("write");
        close(fd);
        return;
    }
    close(fd);
}

void scan_directory_entries(u32 current_cluster) {
    // now we make sure that the current cluster is a directory file 
    // which contains many bmp files entry, so we can scan them to recover each bmp file

    // we assume the directory file is not pass over the current cluster 
    char *cluster_addr = (char *) mapped_image + current_cluster * bpb->BPB_BytsPerSec;
    for (int i = 0; i < bpb->BPB_SecPerClus * bpb->BPB_BytsPerSec; i+=32 ) {
        // loop through each entry of the cluster 
         
        char *entry = cluster_addr + i;
        if (entry[8] == 'B' && entry[9] == 'M' && entry[10] == 'P') {
            // this is a bmp file entry, we can get its name , size and first cluster ...
            char name[64]; // maybe a long name ... but now we just assume it is a short name
            u32 bmp_size = *(u32*)(entry + 28);
            u32 first_cluster = *(u16*)(entry + 26) | (*(u16*)(entry + 20) << 16);

            for (int j = 0; j < 12; j++) {
                if (j < 8) {
                    name[j] = entry[j];
                } else if (j == 8) {
                    name[j] = '.';
                } else {
                    name[j] = entry[j-1];
                }
            }
            name[12] = '\0';

            printf("Found BMP file: %s, size: %u, first cluster: %u\n", name, bmp_size, first_cluster);

            // Now we can recover this bmp file by reading its data from the first cluster and its size
            // Of course, this is based on the assumption that the bmp file is continuous if its size is larger than a cluster 
            // And we can check if its header is a bmp header 

            get_bmp(first_cluster, bmp_size, name);

        }
        
    }
}

int main(int argc, char *argv[]) {

    // Now , fsrecov.img is in the current directory
    // Step 1: mmap the fsrecov.img to memory 

    // Get the file name from the command line argument 
    // For example, ./fsrecov xxx.img
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image.img>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *image_file = argv[1];

    printf("Recovering BMP files from image: %s\n", image_file);

    // mmap the image file to memory 
    // get the file size, we assuem the image file is less than 128 MB

    int fd;
    size_t length;
    struct stat sb;

    fd = open(image_file, O_RDONLY);

    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // offset must be a multiple of the page size, but here is 0 
    // so do not worry about it 

    length = sb.st_size;

    // set the protection to read only, and the mapping is private 
    // because we do not want to write back to the image file ...
    mapped_image = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);

    if (mapped_image == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Now we get the address of the mapped image file : mapped_image
    // Step 2: parse the first sector of the image file 
    // The first sector is 512 bytes, and it contains the BPB info

    bpb = (struct fat32hdr *)mapped_image;

    // maybe we can firstly print some basic info of the BPB to check if we mmap and parse the image file correctly 
    printf("BPB_BytsPerSec: %u\n", bpb->BPB_BytsPerSec);
    printf("BPB_SecPerClus: %u\n", bpb->BPB_SecPerClus);
    printf("BPB_RsvdSecCnt: %u\n", bpb->BPB_RsvdSecCnt);
    printf("BPB_NumFATs: %u\n", bpb->BPB_NumFATs);
    printf("BPB_RootClus: %u\n", bpb->BPB_RootClus);
    printf("BPB_FATSz32: %u\n", bpb->BPB_FATSz32);
    printf("BPB_TotSec32: %u\n", bpb->BPB_TotSec32);
    printf("BPB_FSInfo: %u\n", bpb->BPB_FSInfo);
    printf("BPB_BkBootSec: %u\n", bpb->BPB_BkBootSec);

    // It seems that the BPB info is correct, so now let's scan the data clusters from BPB_RootClus to last cluster
    // Step 3: scan from BPB_RootClus (normally is 2)

    // maybe we need to firstly calculate the address of the first data cluster 
    // There are reserved sectors, some FAT tables, and then the data area starts
    // Notice: for fat32, the root directory is a cluster chain, not a fixed area !

    // Calculate the address of the first data cluster
    u32 first_data_sector = (u32)bpb->BPB_RsvdSecCnt+ (u32)bpb->BPB_NumFATs * bpb->BPB_FATSz32;

    // because the first data cluster is cluster 2 , so this is the start address of scanning 

    u32 current_cluster = (bpb->BPB_RootClus - 2) * (u32)bpb->BPB_SecPerClus + first_data_sector;

    // calculate the count of clusters 
    u32 DataSec = bpb->BPB_TotSec32 - first_data_sector;
    u32 CountOfClusters = DataSec / bpb->BPB_SecPerClus;
    u32 current_cluster_index = bpb->BPB_RootClus;

    printf("The count of clusters in data area: %u\n", CountOfClusters);

    // Start to scan 
    while (1)
    {
        // divide the current cluster to 4 types:
        // here maybe we can only check if it is a directory file ...
        // All bmp files can be solved in the directory entry ...

        int flag = check_directory_file(current_cluster);

        if (flag) {
            // is a directory file, we can loop through the directory entries to 
            // get each bmp file's name, size and first cluster number ...

            // There exists a problem that the directory may have some sub-directory ?
            // no worry about it , 如果真的有子目录，在遍历路上总会再次碰上的

            // so let's just scan the directory entry 

            // but there is a problem that the directory may pass over the current cluster 
            // and we don't know the next cluster number because we lost the FAT table.

            scan_directory_entries(current_cluster);
            

        } else {
            // not a directory file, maybe a bmp header or bmp real data ?
            // no worry about it , just pass ...
        }

        // move to the next cluster 
        current_cluster += bpb->BPB_SecPerClus;
        current_cluster_index++;

        if (current_cluster_index > CountOfClusters + 1) {
            // already pass over the last cluster , so stop scanning 
            break;
        }
    }
    

}

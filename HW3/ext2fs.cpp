#include "ext2fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BASE_OFFSET 1024

unsigned int block_size;
int image;
struct ext2_super_block super;

int main(int argc, char **argv){
    if ((image = open(argv[1], O_RDWR)) < 0) {
        fprintf(stderr, "Could not open img file RDWR\n");
        return 1;
    }
    lseek(image, BASE_OFFSET, SEEK_SET);
    read(image, &super, sizeof(super));
    if (super.magic != EXT2_SUPER_MAGIC) {
        fprintf(stderr, "Img file could not be classified as a ext2 fs\n");
        return 1;
    }
    block_size = 1024 << super.log_block_size;


    printf("Inodes count: %d\n", super.inode_count);
    printf("Blocks count: %d\n", super.block_count);
    printf("Reserved blocks count: %d\n", super.reserved_block_count);
    printf("Free blocks count: %d\n", super.free_block_count);
    printf("Free inodes count: %d\n", super.free_inode_count);
    printf("First data block: %d\n", super.first_data_block);
    printf("Block size: %d\n", 1024 << super.log_block_size);



    return 0;
}
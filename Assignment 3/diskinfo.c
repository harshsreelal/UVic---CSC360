#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#define SECTOR_SIZE 512

void getOSName(char* osName, char* p) {
    for (int i = 0; i < 8; i++) {
        osName[i] = p[i+3];
    }
}

void getLabel(char* diskLabel, char* p) {
    for (int i = 0; i < 8; i++) {
        diskLabel[i] = p[i+43];
    }

    if (diskLabel[0] == ' ') {
        p += SECTOR_SIZE * 19;
        while (p[0] != 0x00) {
            if (p[11] == 8) {
                for (int i = 0; i < 8; i++) {
                    diskLabel[i] = p[i];
                }
            }
            p += 32;
        }
    }
}

int getFatEntry(int n, char* p) {
	int result;
	int firstByte;
	int secondByte;

	if ((n % 2) == 0) {
		firstByte = p[SECTOR_SIZE + ((3*n) / 2) + 1] & 0x0F;
		secondByte = p[SECTOR_SIZE + ((3*n) / 2)] & 0xFF;
		result = (firstByte << 8) + secondByte;
	} else {
		firstByte = p[SECTOR_SIZE + (int)((3*n) / 2)] & 0xF0;
		secondByte = p[SECTOR_SIZE + (int)((3*n) / 2) + 1] & 0xFF;
		result = (firstByte >> 4) + (secondByte << 4);
	}

	return result;
}

int getTotalSize(char* p) {
    int bytesPerSector = p[11] + (p[12] << 8);
	int totalSectorCount = p[19] + (p[20] << 8);
	return bytesPerSector * totalSectorCount;
}

int getFreeSize(int size, char* p) {
    int freeSectors = 0;

	int i;
	for (i = 2; i < (size / SECTOR_SIZE); i++) {
		if (getFatEntry(i, p) == 0x000) {
			freeSectors++;
		}
	}

	return SECTOR_SIZE * freeSectors;
}

// void readSubdirectory(char* p, int sector, int* count) {
//     printf("there\n");
//     p += SECTOR_SIZE * sector;
//     getTotalFiles(p, count);
// }

void getTotalFiles(char* p, int* count) {
    // p += SECTOR_SIZE * 19;
	// int count = 0;

	while (p[0] != 0x00) {

        unsigned short firstCluster = *((unsigned short*)(p + 26));
        if (firstCluster == 0 || firstCluster == 1) {
            p += 32;
            continue;
        }

        if ((p[11] & 0b00000010) == 0 && (p[11] & 0b00001000) == 0 && (p[11] & 0b00010000) == 0) {  //Counting files
			printf("file\n");
            (*count)++;
            printf("counted\n");
		} else if ((p[11] & 0b00010000) != 0 && (p[11] & 0b00001000) == 0 && (p[11] & 0b00000010) == 0) {  //Accessing subdirectories
			// printf("here\n");
            printf("dir\n");
            p += (firstCluster + 31 + 1) * SECTOR_SIZE;
            // char* subdir = p + (firstCluster + 31) * SECTOR_SIZE;
            // printf("Subdir pointer: %p\n", subdir);
            getTotalFiles(p, count);
            printf("did\n");
		}
		p += 32;
	}
}

int getFATCopies(char* p) {
    return p[16];
}

int getSectorsPerFAT(char* p) {
	return p[22] + (p[23] << 8);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Error: File not found\n");
        return 1;
    }

    int disk = open(argv[1], O_RDWR);
    if (disk < 0) {
        printf("Error: Failed to read disk image\n");
        return 1;
    }

    struct stat sb;
    fstat(disk, &sb);
    char* p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, disk, 0);
    if (p == MAP_FAILED) {
        printf("Error: Failed to map memory\n");
        return 1;
    }

    char* osName = malloc(sizeof(char));
    getOSName(osName, p);
    char* label = malloc(sizeof(char));
    getLabel(label, p);
    int totalSize = getTotalSize(p);
    int freeSize = getFreeSize(totalSize, p);
    int count = 0;
    getTotalFiles(p + SECTOR_SIZE * 19, &count);
    int numFATCopies = getFATCopies(p);
    int sectorsFAT = getSectorsPerFAT(p);

    printf("OS Name: %s\n", osName);
	printf("Label of the disk: %s\n", label);
	printf("Total size of the disk: %d bytes\n", totalSize);
	printf("Free size of the disk: %d bytes\n\n", freeSize);
	printf("==============\n");
	printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n\n", count);
	printf("=============\n");
	printf("Number of FAT copies: %d\n", numFATCopies);
	printf("Sectors per FAT: %d\n", sectorsFAT);
    
    munmap(p, sb.st_size);
	close(disk);
	free(osName);
	free(label);

	return 0;
}
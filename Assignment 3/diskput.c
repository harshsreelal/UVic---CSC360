#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define SECTOR_SIZE 512
#define TRUE 1
#define FALSE 0

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

int diskContainsFile(char* fileName, char* p) {
	while (p[0] != 0x00) {
		if ((p[11] & 0b00000010) == 0 && (p[11] & 0b00001000) == 0) {
			char* currentFileName = malloc(sizeof(char));
			char* currentFileExtension = malloc(sizeof(char));
			int i;
			for (i = 0; i < 8; i++) {
				if (p[i] == ' ') {
					break;
				}
				currentFileName[i] = p[i];
			}
			for (i = 0; i < 3; i++) {
				currentFileExtension[i] = p[i+8];
			}

			strcat(currentFileName, ".");
			strcat(currentFileName, currentFileExtension);

			if (strcmp(fileName, currentFileName) == 0) {
				return TRUE;
			}
		}
		p += 32;
	}
	return FALSE;
}

void updateRootDirectory(char* fileName, int fileSize, int firstLogicalSector, char* p) {
	p += SECTOR_SIZE * 19;
	while (p[0] != 0x00) {
		p += 32;
	}
	
	int i;
	int done = -1;
	for (i = 0; i < 8; i++) {
		char character = fileName[i];
		printf("%c\n", character);
		if (character == '.') {
			done = i;
		}
		printf("%d\n", done);
		if (done == -1) {
			printf("%c\n", character);
			p[i] = character;
		} else {
			p[i] = ' ';
		}
		// p[i] = (done == -1) ? character : ' ';
	}
	printf("hi\n");
	for (i = 0; i < 3; i++) {
		p[i+8] = fileName[i+done+1];
	}
	
	p[11] = 0x00;

	time_t t = time(NULL);
	struct tm *now = localtime(&t);
	int year = now->tm_year + 1900;
	int month = (now->tm_mon + 1);
	int day = now->tm_mday;
	int hour = now->tm_hour;
	int minute = now->tm_min;
	p[14] = 0;
	p[15] = 0;
	p[16] = 0;
	p[17] = 0;
	p[17] |= (year - 1980) << 1;
	p[17] |= (month - ((p[16] & 0b11100000) >> 5)) >> 3;
	p[16] |= (month - (((p[17] & 0b00000001)) << 3)) << 5;
	p[16] |= (day & 0b00011111);
	p[15] |= (hour << 3) & 0b11111000;
	p[15] |= (minute - ((p[14] & 0b11100000) >> 5)) >> 3;
	p[14] |= (minute - ((p[15] & 0b00000111) << 3)) << 5;

	p[26] = (firstLogicalSector - (p[27] << 8)) & 0xFF;
	p[27] = (firstLogicalSector - p[26]) >> 8;

	p[28] = (fileSize & 0x000000FF);
	p[29] = (fileSize & 0x0000FF00) >> 8;
	p[30] = (fileSize & 0x00FF0000) >> 16;
	p[31] = (fileSize & 0xFF000000) >> 24;
}

int getNextFreeFatIndex(char* p) {
	p += SECTOR_SIZE;

	int n = 2;
	while (getFatEntry(n, p) != 0x000) {
		n++;
	}

	return n;
}

void setFatEntry(int n, int value, char* p) {
	p += SECTOR_SIZE;

	if ((n % 2) == 0) {
		p[SECTOR_SIZE + ((3*n) / 2) + 1] = (value >> 8) & 0x0F;
		p[SECTOR_SIZE + ((3*n) / 2)] = value & 0xFF;
	} else {
		p[SECTOR_SIZE + (int)((3*n) / 2)] = (value << 4) & 0xF0;
		p[SECTOR_SIZE + (int)((3*n) / 2) + 1] = (value >> 4) & 0xFF;
	}
}

void copyFile(char* p, char* fileMap, char* fileName, int fileSize) {
	if (!diskContainsFile(fileName, p + SECTOR_SIZE * 19)) {
		int bytesRemaining = fileSize;
		int current = getNextFreeFatIndex(p);
		updateRootDirectory(fileName, fileSize, current, p);
		printf("hi\n");
		
		while (bytesRemaining > 0) {
			int physicalAddress = SECTOR_SIZE * (31 + current);
			
			int i;
			for (i = 0; i < SECTOR_SIZE; i++) {
				if (bytesRemaining == 0) {
					setFatEntry(current, 0xFFF, p);
					return;
				}
				p[i + physicalAddress] = fileMap[fileSize - bytesRemaining];
				bytesRemaining--;
			}
			setFatEntry(current, 0x69, p);
			int next = getNextFreeFatIndex(p);
			setFatEntry(current, next, p);
			current = next;
		}
	} 
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Error: Invalid Use\n");
		return(1);
	}

    int disk = open(argv[1], O_RDWR);
    if (disk < 0) {
        printf("Error: Failed to read disk image\n");
		close(disk);
        return 1;
    }

    struct stat sb;
    fstat(disk, &sb);
    char* p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, disk, 0);
    if (p == MAP_FAILED) {
        printf("Error: Failed to map memory\n");
        return 1;
    }

	int file = open(argv[2], O_RDWR);
	if (file < 0) {
		printf("File not found.\n");
		close(disk);
		return 1;
	}
	struct stat sb2;
	fstat(file, &sb2);
	int fileSize = sb2.st_size;
	char* fileMap = mmap(NULL, fileSize, PROT_READ, MAP_SHARED, file, 0);
	if (fileMap == MAP_FAILED) {
		printf("Error: Failed to map file memory\n");
		return 1;
	}

	// printf("file\n");
	int totalDiskSize = getTotalSize(p);
	int freeDiskSize = getFreeSize(totalDiskSize, p);
	if (freeDiskSize >= fileSize) {
		copyFile(p, fileMap, argv[2], fileSize);
	} else {
		printf("%d %d\n", freeDiskSize, fileSize);
		printf("Not enough free space in the disk image.\n");
	}

	munmap(p, sb.st_size);
	munmap(fileMap, fileSize);
	close(disk);
	close(file);

	return 0;
}
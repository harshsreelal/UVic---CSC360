#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SECTOR_SIZE 512 

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

int getFileSize(char* fileName, char* p) {
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
				int fileSize = (p[28] & 0xFF) + ((p[29] & 0xFF) << 8) + ((p[30] & 0xFF) << 16) + ((p[31] & 0xFF) << 24);
				return fileSize;
			}

			free(currentFileName);
			free(currentFileExtension);
		}
		p += 32;
	}
	return -1;
}

int getFirstLogicalSector(char* fileName, char* p) {
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
				return (p[26]) + (p[27] << 8);
			}

			// free(currentFileName);
			// free(currentFileExtension);
		}
		p += 32;
	}
	return -1;
}

void copyFile(char* p, char* fileMap, char* fileName) {
	int firstLogicalSector = getFirstLogicalSector(fileName, p + SECTOR_SIZE * 19);
	int n = firstLogicalSector;
	int fileSize = getFileSize(fileName, p + SECTOR_SIZE * 19);
	int bytesRemaining = fileSize;
	int physicalAddress = SECTOR_SIZE * (31 + n);

	do {
		n = (bytesRemaining == fileSize) ? firstLogicalSector : getFatEntry(n, p);
		physicalAddress = SECTOR_SIZE * (31 + n);

		int i;
		for (i = 0; i < SECTOR_SIZE; i++) {
			if (bytesRemaining == 0) {
				break;
			}
			fileMap[fileSize - bytesRemaining] = p[i + physicalAddress];
			bytesRemaining--;
		}
	} while (getFatEntry(n, p) != 0xFFF);
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Error: Invalid Use\n");
		exit(1);
	}

	int disk = open(argv[1], O_RDWR);
	if (disk < 0) {
		printf("Error: Failed to read disk image\n");
		exit(1);
	}
	struct stat buf;
	fstat(disk, &buf);
	char* p = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, disk, 0);
	if (p == MAP_FAILED) {
		printf("Error: Failed to map memory\n");
		exit(1);
	}

	int fileSize = getFileSize(argv[2], p + SECTOR_SIZE * 19);
	if (fileSize > 0) {
		int file = open(argv[2], O_RDWR | O_CREAT, 0666);
		if (file < 0) {
			printf("Error: Failed to open file\n");
			exit(1);
		}

		int result = lseek(file, fileSize-1, SEEK_SET);
		// if (result == -1) {
		// 	munmap(p, buf.st_size);
		// 	close(disk);
		// 	close(file);
		// 	printf("Error: failed to seek to end of file\n");
		// 	exit(1);
		// }
		result = write(file, "", 1);
		// if (result != 1) {
		// 	munmap(p, buf.st_size);
		// 	close(disk);
		// 	close(file);
		// 	printf("Error: failed to write last byte\n");
		// 	exit(1);
		// }

		char* fileMap = mmap(NULL, fileSize, PROT_WRITE, MAP_SHARED, file, 0);
		if (fileMap == MAP_FAILED) {
			printf("Error: failed to map file memory\n");
			exit(1);
		}

		copyFile(p, fileMap, argv[2]);

		munmap(fileMap, fileSize);
		close(file);
	} else {
		printf("File not found.\n");
	}

	munmap(p, buf.st_size);
	close(disk);

	return 0;
}
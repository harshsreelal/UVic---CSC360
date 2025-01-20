#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SECTOR_SIZE 512

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

void printDirectoryListing(char* p) {
	while (p[0] != 0x00) {
		char fileType;
		if ((p[11] & 0b00010000) == 0b00010000) {
			fileType = 'D';
		} else {
			fileType = 'F';
		}

		char* fileName = malloc(sizeof(char));
		int i;
		for (i = 0; i < 8; i++) {
			if (p[i] == ' ') {
				break;
			}
			fileName[i] = p[i];
		}

		char* fileExtension = malloc(sizeof(char));
		for (i = 0; i < 3; i++) {
			fileExtension[i] = p[i+8];
		}

		strcat(fileName, ".");
		strcat(fileName, fileExtension);
		
		int fileSize = getFileSize(fileName, p);

		int year = (((p[17] & 0b11111110)) >> 1) + 1980;
		int month = ((p[16] & 0b11100000) >> 5) + (((p[17] & 0b00000001)) << 3);
		int day = (p[16] & 0b00011111);
		int hour = (p[15] & 0b11111000) >> 3;
		int minute = ((p[14] & 0b11100000) >> 5) + ((p[15] & 0b00000111) << 3);
		if ((p[11] & 0b00000010) == 0 && (p[11] & 0b00001000) == 0) {
			printf("%c %10d %20s %d-%d-%d %02d:%02d\n", fileType, fileSize, fileName, year, month, day, hour, minute);
		}
		p += 32;
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error: Invalid Use\n");
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

    printDirectoryListing(p + SECTOR_SIZE * 19);

	munmap(p, sb.st_size);
	close(disk);

	return 0;
}
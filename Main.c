#include <stdlib.h>
#include "src/include/raylib.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct list {
	char** list;
	size_t size;
} List;

typedef struct content_files {
	List oldFileLinesList;
	List newFileLinesList;
} ContentFiles;

#define WIDTH 1600
#define HEIGHT 900
#define FONT_SIZE 20
#define MARGIN_X 5
#define MARGIN_Y 5
#define PADDING_FROM_LINE_NUMBER MARGIN_X * 6

#define LINE_DELIMETER '\n'

static void cpyStrFromTo(char* dst, char* src, int fromSrc, int upToSrc)
{
	int i = 0;
	while (fromSrc < upToSrc)
	{
		if (src[fromSrc] == '\0')
			break;
		dst[i++] = src[fromSrc++];
	}
}

static List loadLinesIntoList(char* fileContentBuffer, char** fileLines)
{
	char* fileLineBuffer = calloc(1, sizeof(char));

	if (fileLineBuffer == NULL)
	{
		puts("Unable to allocate memory for old file buffer lines.");
		exit(1);
	}

	size_t fileLineBufferIdx = 0;
	size_t fileLinesIdx = 0;

	while (1)
	{
		if (*fileContentBuffer == LINE_DELIMETER || *fileContentBuffer == '\0')
		{
			char* endOfLineBuffer = fileLineBuffer + fileLineBufferIdx;

			if (endOfLineBuffer == NULL)
			{
				puts("Reached memory it shouldn't have.");
				exit(1);
			}

			*endOfLineBuffer = '\0';
			size_t stringSize = fileLineBufferIdx + 1;

			*(fileLines + fileLinesIdx) = calloc(stringSize + 1, sizeof(char));
			if (*(fileLines + fileLinesIdx) == 0) exit(1);

			strcpy_s(*(fileLines + fileLinesIdx), stringSize + 1, fileLineBuffer);
			fileLinesIdx++;

			char** reallocatedOldFileLines = realloc(fileLines, sizeof(char*) * (fileLinesIdx + 1));
			if (reallocatedOldFileLines == NULL)
			{
				puts("Unable to allocate memory for the old file.");
				exit(1);
			}
			fileLines = reallocatedOldFileLines;

			free(fileLineBuffer);
			fileLineBufferIdx = 0;

			if (*fileContentBuffer == '\0')
			{
				break;
			}

			fileLineBuffer = calloc(1, sizeof(char));
			fileContentBuffer++;
			continue;
		}

		char* nextPtrInLineBuffer = (fileLineBuffer + fileLineBufferIdx++);
		if (nextPtrInLineBuffer == NULL)
		{
			puts("Reached memory it shouldn't have.");
			exit(1);
		}
		*nextPtrInLineBuffer = *fileContentBuffer;
		char* oldFileBufferLineReallocated = realloc(fileLineBuffer, fileLineBufferIdx + 1);
		if (oldFileBufferLineReallocated == NULL)
		{
			puts("Unable to allocate memory for the old file buffer line.");
			exit(1);
		}
		fileLineBuffer = oldFileBufferLineReallocated;
		fileContentBuffer++;
	}

	return (List) { .size = fileLinesIdx, .list = fileLines };
}

static ContentFiles* loadFilesIntoMemory(const char* oldFileBuffer, const char* newFileBuffer, const size_t oldFileSizeInBytes, const size_t newFileSizeInBytes)
{
	char* oldFileBufferCpy = calloc(oldFileSizeInBytes + 1, sizeof(char));
	if (oldFileBufferCpy == NULL) exit(1);
	strcpy_s(oldFileBufferCpy, oldFileSizeInBytes + 1, oldFileBuffer);

	char* newFileBufferCpy = calloc(newFileSizeInBytes + 1, sizeof(char));
	if (newFileBufferCpy == NULL) exit(1);
	strcpy_s(newFileBufferCpy, newFileSizeInBytes + 1, newFileBuffer);

	char* oldFileBufferCpyPtr = oldFileBufferCpy;
	char* newFileBufferCpyPtr = newFileBufferCpy;

	char** oldFileLines = calloc(1, sizeof(char*));
	char** newFileLines = calloc(1, sizeof(char*));

	if (oldFileLines == NULL || newFileLines == NULL)
	{
		puts("Unable to allocate memory for one or both files.");
		exit(1);
	}

	size_t oldFileLinesIdx = 0;
	size_t newFileLinesIdx = 0;

	char* oldFileBufferLine = calloc(1, sizeof(char));

	if (oldFileBufferLine == NULL)
	{
		puts("Unable to allocate memory for old file buffer lines.");
		exit(1);
	}

	List oldFileList = loadLinesIntoList(oldFileBufferCpy, oldFileLines);
	List newFileList = loadLinesIntoList(newFileBufferCpy, newFileLines);

	free(oldFileBufferCpyPtr);
	free(newFileBufferCpyPtr);

	ContentFiles* contentFiles = malloc(sizeof(ContentFiles));
	
	if (contentFiles == NULL)
	{
		puts("Unable to allocate memory for content files struct.");
		exit(1);
	}

	contentFiles->oldFileLinesList = oldFileList;
	contentFiles->newFileLinesList = newFileList;

	return contentFiles;
}

static int contains(int* list, size_t size, int item)
{
	for (size_t i = 0; i < size; i++)
	{
		if (list[i] == item)
			return 1;
	}

	return 0;
}

static void addToSeenLines(int* seenLines, size_t* seenLinesSize, int line)
{
	seenLines[(*seenLinesSize)++] = line;
}

static int lineDoesNotExistInFile(const List* fileLines, const char** lineToFind)
{
	for (size_t i = 0; i < fileLines->size; i++)
	{
		if (strcmp(fileLines->list[i], *lineToFind) == 0 || (strlen(fileLines->list[i]) == 0 && strlen(*lineToFind) == 0))
		{
			return 0;
		}
	}
	return 1;
}

static int lineDoesNotExistInFileAndHasNotBeenSeen(const List* fileLines, const char** lineToFind, int* seenLines, size_t* seenLinesSize, int line)
{
	for (size_t i = 0; i < fileLines->size; i++)
	{
		if (strcmp(fileLines->list[i], *lineToFind) == 0 && !seenLine(seenLines, *seenLinesSize, i + 1) || (strlen(fileLines->list[i]) == 0 && strlen(*lineToFind) == 0))
		{
			addToSeenLines(seenLines, seenLinesSize, (int)i + 1);
			return 0;
		}
	}
	return 1;
}

static void drawLineNumber(int number, int posX, int posY)
{
	char lineNumber[6] = { 0 };
	sprintf_s(lineNumber, 6, "%d", number);
	DrawText(lineNumber, posX, posY, FONT_SIZE, (Color) { .a = 255, .r = 50, .g = 50, .b = 50 });
}

static int seenLine(int* seenLines, size_t seenLinesSize, int line)
{
	for (size_t i = 0; i < seenLinesSize; i++)
	{
		if (seenLines[i] == line)
		{
			return 1;
		}
	}

	return 0;
}

int main(void)
{
	InitWindow(WIDTH, HEIGHT, "TextComparer");
	SetTargetFPS(30);

	// Load font
	Font robotoMonoFont = LoadFont("resources/RobotoMono-VariableFont_wght.ttf");

	// opening old file
	FILE* oldFile = fopen("resources/old_file.txt", "r");
	fseek(oldFile, 0, SEEK_END);
	size_t oldFileSizeInBytes = ftell(oldFile);
	rewind(oldFile);
	char* oldFileBuffer = calloc(oldFileSizeInBytes + 1, sizeof(char));
	if (oldFileBuffer == 0) {
		puts("Unable to allocate memory for old file buffer");
		exit(1);
	}
	fread(oldFileBuffer, sizeof(char), oldFileSizeInBytes, oldFile);

	// opening new file
	FILE* newFile = fopen("resources/new_file.txt", "r");
	fseek(newFile, 0, SEEK_END);
	size_t newFileSizeInBytes = ftell(newFile);
	rewind(newFile);
	char* newFileBuffer = calloc(newFileSizeInBytes + 1, sizeof(char));
	if (newFileBuffer == 0) {
		puts("Unable to allocate memory for new file buffer");
		exit(1);
	}
	fread(newFileBuffer, sizeof(char), newFileSizeInBytes, newFile);

	// Load file content into lists
	ContentFiles* contentFiles = loadFilesIntoMemory(oldFileBuffer, newFileBuffer, oldFileSizeInBytes, newFileSizeInBytes);

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

		DrawLine(WIDTH / 2, 0, WIDTH / 2, HEIGHT, WHITE);

		int posX = MARGIN_X;
		int posY = MARGIN_Y;
		// TODO: It feels as something is wrong with either the position of the
		// line that divides the comparison or this math itself.
		const size_t canFitUpToChars = floor(WIDTH / FONT_SIZE);

		// TODO: this should be realloc'ed
		int seenLines[_MAX_PATH] = { 0 };
		size_t seenLinesSize = 0;

		// Old file lines
		for (size_t i = 0; i < contentFiles->oldFileLinesList.size; i++)
		{
			size_t lineSize = strlen(contentFiles->oldFileLinesList.list[i]);

			if (lineSize > canFitUpToChars)
			{
				// Draw the whole line in separate rendered lines.
				size_t lineIdx = 0;
				int renderedLines = 0;
				int alreadyDrawnLineNumber = 0;
				int originalPosY = posY;

				while (lineIdx < lineSize)
				{
					char partOfLineBuffer[(WIDTH / FONT_SIZE) + 1] = { 0 };
					size_t partOfLineBufferIdx = 0;

					while (partOfLineBufferIdx < canFitUpToChars && lineIdx < lineSize)
						partOfLineBuffer[partOfLineBufferIdx++] = contentFiles->oldFileLinesList.list[i][lineIdx++];

					DrawTextEx(robotoMonoFont, partOfLineBuffer, (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);
					
					if (!alreadyDrawnLineNumber)
					{
						drawLineNumber((int)i + 1, posX, posY);
						alreadyDrawnLineNumber = 1;
					}

					posY += FONT_SIZE + MARGIN_Y;
					renderedLines++;
				}

				if (lineDoesNotExistInFile(&contentFiles->newFileLinesList, &contentFiles->oldFileLinesList.list[i]))
				{
					DrawRectangle(posX, originalPosY, WIDTH / 2, (FONT_SIZE * renderedLines) + MARGIN_Y, (Color) { .a = 50, .r = 200, .g = 0, .b = 0 });
				}

				continue;
			}

			if (lineDoesNotExistInFile(&contentFiles->newFileLinesList, &contentFiles->oldFileLinesList.list[i]))
			{
				DrawRectangle(posX, posY, WIDTH / 2, FONT_SIZE, (Color) { .a = 50, .r = 200, .g = 0, .b = 0 });
			}

			drawLineNumber((int)i + 1, posX, posY);

			DrawTextEx(robotoMonoFont, contentFiles->oldFileLinesList.list[i], (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);
			posY += FONT_SIZE + MARGIN_Y;
		}

		posX = (WIDTH / 2) + MARGIN_X;
		posY = MARGIN_Y;

		// New file lines
		for (size_t i = 0; i < contentFiles->newFileLinesList.size; i++)
		{
			if (lineDoesNotExistInFileAndHasNotBeenSeen(&contentFiles->oldFileLinesList, &contentFiles->newFileLinesList.list[i], seenLines, &seenLinesSize, i + 1))
			{
				DrawRectangle(posX, posY, WIDTH / 2, FONT_SIZE, (Color) { .a = 50, .r = 0, .g = 200, .b = 0 });
			}

			drawLineNumber((int)i + 1, posX, posY);

			DrawTextEx(robotoMonoFont, contentFiles->newFileLinesList.list[i], (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);
			posY += FONT_SIZE + MARGIN_Y;
		}

		EndDrawing();
	}

	// cleanup
	for (size_t i = 0; i < contentFiles->oldFileLinesList.size; i++)
	{
		free(contentFiles->oldFileLinesList.list[i]);
	}

	for (size_t i = 0; i < contentFiles->newFileLinesList.size; i++)
	{
		free(contentFiles->newFileLinesList.list[i]);
	}

	free(contentFiles);
	free(newFileBuffer);
	free(oldFileBuffer);
	fclose(oldFile);
	fclose(newFile);

	CloseWindow();

	return EXIT_SUCCESS;
}
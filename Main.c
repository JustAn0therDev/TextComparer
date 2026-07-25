#include <stdlib.h>
#include "src/include/raylib.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct list {
	char** list;
	size_t size;
} List;

typedef struct number_list {
	size_t* list;
	size_t size;
} NumberList;

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
#define FITS_UP_TO_CHARS (WIDTH / 2) - (MARGIN_Y * 2) - PADDING_FROM_LINE_NUMBER
#define MAX_RENDERED_LINE_SIZE 2048

#define LINE_DELIMETER '\n'

static char* readFileToBuffer(const char* filePath, size_t* readFileSize);
static void cpyStrFromTo(char* dst, char* src, int fromSrc, int upToSrc);
static List loadLinesIntoList(char* fileContentBuffer, char** fileLines);
static ContentFiles* loadFilesIntoMemory(const char* oldFileBuffer, const char* newFileBuffer, const size_t oldFileSizeInBytes, const size_t newFileSizeInBytes);
static void addToSeenLines(NumberList* numberList, int line);
static int lineDoesNotExistInFile(const List* fileLines, const char** lineToFind);
static int lineDoesNotExistInFileAndHasNotBeenSeen(const List* fileLines, const char** lineToFind, NumberList* seenLines, int line);
static int seenLine(NumberList* seenLines, int line);
static void drawLineNumber(int number, int posX, int posY);
static void renderOldFileLines(const ContentFiles* contentFiles, Font font);
static void renderNewFileLines(const ContentFiles* contentFiles, Font font);

static char* readFileToBuffer(const char* filePath, size_t* readFileSize)
{
	FILE* file = fopen(filePath, "r");
	fseek(file, 0, SEEK_END);
	*readFileSize = ftell(file);
	rewind(file);
	char* fileBuffer = calloc((*readFileSize) + 1, sizeof(char));
	if (fileBuffer == 0) {
		puts("Unable to allocate memory for old file buffer");
		exit(1);
	}
	fread(fileBuffer, sizeof(char), *readFileSize, file);
	fclose(file);

	return fileBuffer;
}

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

static void renderOldFileLines(const ContentFiles* contentFiles, Font font)
{
	int posX = MARGIN_X;
	int posY = MARGIN_Y;

	// Old file lines
	for (size_t i = 0; i < contentFiles->oldFileLinesList.size; i++)
	{
		size_t lineSize = strlen(contentFiles->oldFileLinesList.list[i]);
		Vector2 measuredText = MeasureTextEx(font, contentFiles->oldFileLinesList.list[i], FONT_SIZE, 0);

		if (measuredText.x > FITS_UP_TO_CHARS)
		{
			size_t lineIdx = 0;
			int renderedLines = 0;
			int alreadyDrawnLineNumber = 0;
			int originalPosY = posY;

			while (lineIdx < lineSize)
			{
				char partOfLineBuffer[MAX_RENDERED_LINE_SIZE] = { 0 };
				size_t partOfLineBufferIdx = 0;
				Vector2 measuredPartOfLineText = MeasureTextEx(font, partOfLineBuffer, FONT_SIZE, 0);

				while (measuredPartOfLineText.x <= FITS_UP_TO_CHARS && lineIdx < lineSize)
				{
					partOfLineBuffer[partOfLineBufferIdx++] = contentFiles->oldFileLinesList.list[i][lineIdx++];
					measuredPartOfLineText = MeasureTextEx(font, partOfLineBuffer, FONT_SIZE, 0);
				}

				DrawTextEx(font, partOfLineBuffer, (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);

				if (!alreadyDrawnLineNumber)
				{
					drawLineNumber((int)i + 1, posX, posY);
					alreadyDrawnLineNumber = 1;
				}

				posY += measuredText.y + MARGIN_Y;
				renderedLines++;
			}

			if (lineDoesNotExistInFile(&contentFiles->newFileLinesList, &contentFiles->oldFileLinesList.list[i]))
			{
				DrawRectangle(posX, originalPosY, WIDTH / 2, (measuredText.y * renderedLines) + MARGIN_Y, (Color) { .a = 50, .r = 200, .g = 0, .b = 0 });
			}

			continue;
		}

		if (lineDoesNotExistInFile(&contentFiles->newFileLinesList, &contentFiles->oldFileLinesList.list[i]))
		{
			DrawRectangle(posX, posY, WIDTH / 2, measuredText.y, (Color) { .a = 50, .r = 200, .g = 0, .b = 0 });
		}

		drawLineNumber((int)i + 1, posX, posY);

		DrawTextEx(font, contentFiles->oldFileLinesList.list[i], (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);
		posY += measuredText.y + MARGIN_Y;
	}
}

static void renderNewFileLines(const ContentFiles* contentFiles, Font font)
{
	int posX = (WIDTH / 2) + MARGIN_X;
	int posY = MARGIN_Y;

	NumberList seenLines;
	seenLines.list = calloc(1, sizeof(size_t));
	seenLines.size = 1;

	for (size_t i = 0; i < contentFiles->newFileLinesList.size; i++)
	{
		size_t lineSize = strlen(contentFiles->newFileLinesList.list[i]);
		Vector2 measuredText = MeasureTextEx(font, contentFiles->newFileLinesList.list[i], FONT_SIZE, 0);

		if (measuredText.x > FITS_UP_TO_CHARS)
		{
			size_t lineIdx = 0;
			int renderedLines = 0;
			int alreadyDrawnLineNumber = 0;
			int originalPosY = posY;

			while (lineIdx < lineSize)
			{
				char partOfLineBuffer[MAX_RENDERED_LINE_SIZE] = { 0 };
				size_t partOfLineBufferIdx = 0;
				Vector2 measuredPartOfLineText = MeasureTextEx(font, partOfLineBuffer, FONT_SIZE, 0);

				while (measuredPartOfLineText.x <= FITS_UP_TO_CHARS && lineIdx < lineSize)
				{
					partOfLineBuffer[partOfLineBufferIdx++] = contentFiles->newFileLinesList.list[i][lineIdx++];
					measuredPartOfLineText = MeasureTextEx(font, partOfLineBuffer, FONT_SIZE, 0);
				}

				DrawTextEx(font, partOfLineBuffer, (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);

				if (!alreadyDrawnLineNumber)
				{
					drawLineNumber((int)i + 1, posX, posY);
					alreadyDrawnLineNumber = 1;
				}

				posY += measuredPartOfLineText.y + MARGIN_Y;
				renderedLines++;
			}

			if (lineDoesNotExistInFileAndHasNotBeenSeen(&contentFiles->oldFileLinesList, &contentFiles->newFileLinesList.list[i], &seenLines, i + 1))
			{
				DrawRectangle(posX, originalPosY, WIDTH / 2, (FONT_SIZE * renderedLines) + MARGIN_Y, (Color) { .a = 50, .r = 0, .g = 200, .b = 0 });
			}

			continue;
		}

		drawLineNumber((int)i + 1, posX, posY);

		if (lineDoesNotExistInFileAndHasNotBeenSeen(&contentFiles->oldFileLinesList, &contentFiles->newFileLinesList.list[i], &seenLines, i + 1))
		{
			DrawRectangle(posX, posY, WIDTH / 2, measuredText.y, (Color) { .a = 50, .r = 0, .g = 200, .b = 0 });
		}

		DrawTextEx(font, contentFiles->newFileLinesList.list[i], (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);
		posY += measuredText.y + MARGIN_Y;
	}

	free(seenLines.list);
}

static void drawLineNumber(int number, int posX, int posY)
{
	char lineNumber[6] = { 0 };
	sprintf_s(lineNumber, 6, "%d", number);
	DrawText(lineNumber, posX, posY, FONT_SIZE, (Color) { .a = 255, .r = 50, .g = 50, .b = 50 });
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

static int lineDoesNotExistInFileAndHasNotBeenSeen(const List* fileLines, const char** lineToFind, NumberList* seenLines, int line)
{
	for (size_t i = 0; i < fileLines->size; i++)
	{
		if (strcmp(fileLines->list[i], *lineToFind) == 0 && !seenLine(seenLines, i + 1) || (strlen(fileLines->list[i]) == 0 && strlen(*lineToFind) == 0))
		{
			addToSeenLines(seenLines, (int)i + 1);
			return 0;
		}
	}
	return 1;
}

static int seenLine(NumberList* seenLines, int line)
{
	for (size_t i = 0; i < seenLines->size; i++)
	{
		if (seenLines->list[i] == line)
		{
			return 1;
		}
	}

	return 0;
}

static void addToSeenLines(NumberList* numberList, int line)
{
	numberList->list[numberList->size - 1] = line;
	size_t* reallocated = realloc(numberList->list, sizeof(size_t) * ++numberList->size);
	numberList->list = reallocated;
}

int main(void)
{
	InitWindow(WIDTH, HEIGHT, "TextComparer");

	Font robotoMonoFont = LoadFont("resources/RobotoMono-VariableFont_wght.ttf");

	size_t oldFileSizeInBytes = 0;
	char* oldFileBuffer = readFileToBuffer("resources/old_file.txt", &oldFileSizeInBytes);

	size_t newFileSizeInBytes = 0;
	char* newFileBuffer = readFileToBuffer("resources/new_file.txt", &newFileSizeInBytes);

	ContentFiles* contentFiles = loadFilesIntoMemory(oldFileBuffer, newFileBuffer, oldFileSizeInBytes, newFileSizeInBytes);

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

		DrawLine(WIDTH / 2, 0, WIDTH / 2, HEIGHT, WHITE);

		renderOldFileLines(contentFiles, robotoMonoFont);

		// New file lines
		renderNewFileLines(contentFiles, robotoMonoFont);

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

	CloseWindow();

	return EXIT_SUCCESS;
}
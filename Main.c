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
	int* list;
	size_t size;
} NumberList;

typedef struct content_files {
	List oldFileLinesList;
	List newFileLinesList;
} ContentFiles;

#define WIDTH 1280
#define HEIGHT 720
#define FONT_SIZE 20
#define MARGIN_X 5
#define MARGIN_Y 5
#define PADDING_FROM_LINE_NUMBER MARGIN_X * 10
#define FITS_UP_TO_CHARS (WIDTH / 2) - (MARGIN_X * 2) - PADDING_FROM_LINE_NUMBER
#define MAX_RENDERED_LINE_SIZE 2048
#ifdef _DEBUG
// NOTE: This is for my setup. You don't have to call the SetWindowMonitor function if you don't want to.
#define SECONDARY_MONITOR_ID 1
#endif
#define LINE_DELIMETER '\n'
#define SCROLL_THICKNESS WIDTH * 0.01
#define SCROLL_HEIGHT HEIGHT * 0.2
#define SCROLL_BY_LINES 10
#define LINE_HEIGHT FONT_SIZE + MARGIN_Y
#define MAX_RENDERABLE_LINES (int)ceil((double)HEIGHT / (LINE_HEIGHT))
#define CHAR_SIZE 9
#define MAX_BUFFER_FIT (FITS_UP_TO_CHARS) / CHAR_SIZE
#define MAX_FILEPATHS 2

static char* readFileToBuffer(const char* filePath, size_t* readFileSize);
static List loadLinesIntoList(char* fileContentBuffer, char** fileLines);
static ContentFiles* loadFilesIntoMemory(const char* oldFileBuffer, const char* newFileBuffer, const size_t oldFileSizeInBytes, const size_t newFileSizeInBytes);
static int lineInFileIsDifferent(const List* fileLines, size_t lineIndex, const char** lineToFind);
static void drawLineNumber(const Font* font, int number, int posX, int posY);
static void renderOldFileLines(const ContentFiles* contentFiles, const float totalLinesToRender, const NumberList* oldFileLinesToHighlight, const Font* font, int* fileScrollIndex, int* renderedFinalLine);
static void renderNewFileLines(const ContentFiles* contentFiles, const float totalLinesToRender, const NumberList* newFileLinesToHighlight, const Font* font, int* fileScrollIndex, int* renderedFinalLine);
static int normalizeTextHeightIfLineIsEmpty(Vector2* measuredText);
static void setScrollIndexBasedOnMouseWheelMovement(int* oldFileScrollIndex, int* newFileScrollIndex, int* renderedFinalOldFileLine, int* renderedFinalNewFileLine);
static int getNumberOfLinesThatWillBeRendered(const List* fileLines, const Font* font);
static void drawScroll(const List* fileLines, const float totalLinesToRender, const Font* font, const size_t fileScrollIndex, const int scrollPosX, const int posY);
static void findChangedLinesInOldFile(NumberList* numberList, const ContentFiles* contentFiles);
static void findChangedLinesInNewFile(NumberList* numberList, const ContentFiles* contentFiles);
static void addToNumberList(NumberList* numberList, int index);
static int numberListContains(NumberList* numberList, int index);

static char* readFileToBuffer(const char* filePath, size_t* readFileSize)
{
	FILE* file = fopen(filePath, "r");
	fseek(file, 0, SEEK_END);
	*readFileSize = ftell(file);
	rewind(file);
	char* fileBuffer = calloc((*readFileSize) + 1, sizeof(char));
	if (fileBuffer == 0)
	{
		puts("Unable to allocate memory for old file buffer");
		exit(1);
	}
	fread(fileBuffer, sizeof(char), *readFileSize, file);
	fclose(file);

	return fileBuffer;
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

			char** reallocatedFileLines = realloc(fileLines, sizeof(char*) * (fileLinesIdx + 1));

			if (reallocatedFileLines == NULL)
			{
				puts("Unable to allocate memory for file.");
				exit(1);
			}
			fileLines = reallocatedFileLines;

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
		char* fileBufferLineReallocated = realloc(fileLineBuffer, fileLineBufferIdx + 1);
		if (fileBufferLineReallocated == NULL)
		{
			puts("Unable to allocate memory for file buffer line.");
			exit(1);
		}
		fileLineBuffer = fileBufferLineReallocated;
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

static void renderOldFileLines(const ContentFiles* contentFiles, const float totalLinesToRender, const NumberList* oldFileLinesToHighlight, Font* font, int* fileScrollIndex, int* renderedFinalLine)
{
	int posX = MARGIN_X;
	int posY = MARGIN_Y;

	const int upToLines = (size_t)(*fileScrollIndex) + MAX_RENDERABLE_LINES;

	for (size_t i = *fileScrollIndex; i < contentFiles->oldFileLinesList.size && i < upToLines; i++)
	{
		size_t lineSize = strlen(contentFiles->oldFileLinesList.list[i]);

		if (lineSize > MAX_BUFFER_FIT)
		{
			size_t lineIdx = 0;
			int renderedLines = 0;
			int alreadyDrawnLineNumber = 0;
			int originalPosY = posY;

			while (lineIdx < lineSize)
			{
				char partOfLineBuffer[MAX_BUFFER_FIT + 1] = { 0 };
				size_t partOfLineBufferIdx = 0;

				while (partOfLineBufferIdx < MAX_BUFFER_FIT && lineIdx < lineSize)
				{
					partOfLineBuffer[partOfLineBufferIdx++] = contentFiles->oldFileLinesList.list[i][lineIdx++];
				}

				DrawTextEx(*font, partOfLineBuffer, (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);

				if (!alreadyDrawnLineNumber)
				{
					drawLineNumber(font, (int)i + 1, posX, posY);
					alreadyDrawnLineNumber = 1;
				}

				posY += LINE_HEIGHT;
				renderedLines++;
			}

			if (numberListContains(oldFileLinesToHighlight, i))
			{
				DrawRectangle(posX, originalPosY, (WIDTH / 2) - MARGIN_X, (LINE_HEIGHT)*renderedLines, (Color) { .a = 50, .r = 200, .g = 0, .b = 0 });
			}

			continue;
		}

		drawLineNumber(font, (int)i + 1, posX, posY);

		if (numberListContains(oldFileLinesToHighlight, i))
		{
			DrawRectangle(posX, posY, (WIDTH / 2) - MARGIN_X, LINE_HEIGHT, (Color) { .a = 50, .r = 200, .g = 0, .b = 0 });
		}

		DrawTextEx(*font, contentFiles->oldFileLinesList.list[i], (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);
		posY += LINE_HEIGHT;
	}

	drawScroll(&contentFiles->oldFileLinesList, totalLinesToRender, font, *fileScrollIndex, (WIDTH / 2) - SCROLL_THICKNESS, posY);

	*renderedFinalLine = posY < HEIGHT;
}

static void renderNewFileLines(const ContentFiles* contentFiles, const float totalLinesToRender, const NumberList* newFileLinesToHighlight, Font* font, int* fileScrollIndex, int* renderedFinalLine)
{
	int posX = (WIDTH / 2) + MARGIN_X;
	int posY = MARGIN_Y;
	const int upToLines = (size_t)(*fileScrollIndex) + MAX_RENDERABLE_LINES;

	for (size_t i = *fileScrollIndex; i < contentFiles->newFileLinesList.size && i < upToLines; i++)
	{
		size_t lineSize = strlen(contentFiles->newFileLinesList.list[i]);

		if (lineSize > MAX_BUFFER_FIT)
		{
			size_t lineIdx = 0;
			int renderedLines = 0;
			int alreadyDrawnLineNumber = 0;
			int originalPosY = posY;

			while (lineIdx < lineSize)
			{
				char partOfLineBuffer[MAX_BUFFER_FIT + 1] = { 0 };
				size_t partOfLineBufferIdx = 0;

				while (partOfLineBufferIdx < MAX_BUFFER_FIT && lineIdx < lineSize)
				{
					partOfLineBuffer[partOfLineBufferIdx++] = contentFiles->newFileLinesList.list[i][lineIdx++];
				}

				DrawTextEx(*font, partOfLineBuffer, (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);

				if (!alreadyDrawnLineNumber)
				{
					drawLineNumber(font, (int)i + 1, posX, posY);
					alreadyDrawnLineNumber = 1;
				}

				posY += LINE_HEIGHT;
				renderedLines++;
			}

			if (numberListContains(newFileLinesToHighlight, i))
			{
				DrawRectangle(posX, originalPosY, (WIDTH / 2) - MARGIN_X, (LINE_HEIGHT)*renderedLines, (Color) { .a = 50, .r = 0, .g = 200, .b = 0 });
			}

			continue;
		}

		drawLineNumber(font, (int)i + 1, posX, posY);

		if (numberListContains(newFileLinesToHighlight, i))
		{
			DrawRectangle(posX, posY, (WIDTH / 2) - MARGIN_X, LINE_HEIGHT, (Color) { .a = 50, .r = 0, .g = 200, .b = 0 });
		}

		DrawTextEx(*font, contentFiles->newFileLinesList.list[i], (Vector2) { .x = posX + PADDING_FROM_LINE_NUMBER, .y = posY }, FONT_SIZE, 0, WHITE);
		posY += LINE_HEIGHT;
	}

	drawScroll(&contentFiles->newFileLinesList, totalLinesToRender, font, *fileScrollIndex, WIDTH - SCROLL_THICKNESS, posY);

	*renderedFinalLine = posY < HEIGHT;
}

static void drawLineNumber(Font* font, int number, int posX, int posY)
{
	char lineNumber[6] = { 0 };
	sprintf_s(lineNumber, 6, "%d", number);
	DrawTextEx(*font, lineNumber, (Vector2) { .x = posX, .y = posY }, FONT_SIZE, 0, (Color) { .a = 255, .r = 50, .g = 50, .b = 50 });
}

static int lineInFileIsDifferent(const List* fileLines, size_t lineIndex, const char** lineToFind)
{
	return fileLines->size <= lineIndex || strcmp(fileLines->list[lineIndex], *lineToFind) != 0;
}

static int normalizeTextHeightIfLineIsEmpty(Vector2* measuredText)
{
	if (measuredText->y == 0)
	{
		measuredText->y = FONT_SIZE;
	}
}

static void setScrollIndexBasedOnMouseWheelMovement(int* oldFileScrollIndex, int* newFileScrollIndex, int* renderedFinalOldFileLine, int* renderedFinalNewFileLine)
{
	Vector2 mousePos = GetMousePosition();
	float mouseWheelMovement = GetMouseWheelMove();

	int mouseInOldFileWindow = mousePos.x < WIDTH / 2 && mousePos.y <= HEIGHT;
	int mouseInNewFileWindow = mousePos.x >= WIDTH / 2 && mousePos.y <= HEIGHT;

	if (mouseInOldFileWindow) {
		if (mouseWheelMovement < 0 && !(*renderedFinalOldFileLine)) {
			(*oldFileScrollIndex) += SCROLL_BY_LINES;
		}
		else if (mouseWheelMovement > 0) {
			*oldFileScrollIndex = *oldFileScrollIndex == 0 ? 0 : *oldFileScrollIndex - SCROLL_BY_LINES;
		}
	}
	else if (mouseInNewFileWindow) {
		if (mouseWheelMovement < 0 && !(*renderedFinalNewFileLine)) {
			(*newFileScrollIndex) += SCROLL_BY_LINES;
		}
		else if (mouseWheelMovement > 0) {
			*newFileScrollIndex = *newFileScrollIndex == 0 ? 0 : *newFileScrollIndex - SCROLL_BY_LINES;
		}
	}
}

static int getNumberOfLinesThatWillBeRendered(const List* fileLines, const Font* font)
{
	int lines = 0;

	for (size_t i = 0; i < fileLines->size; i++)
	{
		size_t lineSize = strlen(fileLines->list[i]);
		if (lineSize > MAX_BUFFER_FIT)
		{
			lines += ceil((double)(lineSize / (MAX_BUFFER_FIT)));
		}
		else
		{
			++lines;
		}
	}

	return lines;
}

static void drawScroll(const List* fileLines, const float totalLinesToRender, const Font* font, const size_t fileScrollIndex, const int scrollPosX, const int posY)
{
	if (totalLinesToRender > MAX_RENDERABLE_LINES)
	{
		const int scrollPosY = posY < HEIGHT ? HEIGHT - SCROLL_HEIGHT : fileScrollIndex * ((HEIGHT - SCROLL_HEIGHT) / totalLinesToRender);
		DrawRectangle(scrollPosX, scrollPosY, SCROLL_THICKNESS, SCROLL_HEIGHT, (Color) { .r = 100, .g = 100, .b = 100, .a = 100 });
	}
}

// The whole comparison will have to be re-done.
// Today, it doesn't track empty lines and it does not track the same content twice (no memoization).
static void findChangedLinesInOldFile(NumberList* numberList, const ContentFiles* contentFiles)
{
	for (size_t i = 0; i < contentFiles->oldFileLinesList.size; i++)
	{
		int found = 0;
		char* oldFileLine = contentFiles->oldFileLinesList.list[i];

		for (size_t j = 0; j < contentFiles->newFileLinesList.size; j++)
		{
			char* newFileLine = contentFiles->newFileLinesList.list[j];
			if (strcmp(oldFileLine, newFileLine) == 0 || (strlen(oldFileLine) == 0 && strlen(newFileLine) == 0))
			{
				found = 1;
				break;
			}
		}

		if (!found)
		{
			addToNumberList(numberList, i);
		}
	}
}

static void findChangedLinesInNewFile(NumberList* numberList, const ContentFiles* contentFiles)
{
	for (size_t i = 0; i < contentFiles->newFileLinesList.size; i++)
	{
		int found = 0;
		char* newFileLine = contentFiles->newFileLinesList.list[i];

		for (size_t j = 0; j < contentFiles->oldFileLinesList.size; j++)
		{
			char* oldFileLine = contentFiles->oldFileLinesList.list[j];
			if (strcmp(oldFileLine, newFileLine) == 0 || (strlen(oldFileLine) == 0 && strlen(newFileLine) == 0))
			{
				found = 1;
				break;
			}
		}

		if (!found)
		{
			addToNumberList(numberList, i);
		}
	}
}

static void addToNumberList(NumberList* numberList, int index)
{
	numberList->list[numberList->size] = index;
	int* reallocated = realloc(numberList->list, sizeof(size_t) * ++numberList->size);
	if (reallocated == 0) exit(1);

	numberList->list = reallocated;
}

static int numberListContains(NumberList* numberList, int index)
{
	for (size_t i = 0; i < numberList->size; i++)
	{
		if (numberList->list[i] == index) return 1;
	}

	return 0;
}

int main(void)
{
	InitWindow(WIDTH, HEIGHT, "TextComparer");

#ifdef _DEBUG
	SetWindowMonitor(SECONDARY_MONITOR_ID);
#endif

	// NOTES: 
	// There is absolutely NO need for a text comparer to run at higher frames
	// than 30. So this will be the default for the program, it does the same thing
	// using much, much less resources.
#ifndef _DEBUG
	SetTargetFPS(30);
#endif

	Font robotoMonoFont = LoadFont("resources/RobotoMono-VariableFont_wght.ttf");

	char* oldFileBuffer = 0;
	size_t oldFileSizeInBytes = 0;

	char* newFileBuffer = 0;
	size_t newFileSizeInBytes = 0;

#ifndef _DEBUG
	oldFileBuffer = readFileToBuffer("resources/old_file.txt", &oldFileSizeInBytes);
	newFileBuffer = readFileToBuffer("resources/new_file.txt", &newFileSizeInBytes);
#endif

	int oldFileScrollIndex = 0;
	int newFileScrollIndex = 0;
	int renderedFinalOldFileLine = 0;
	int renderedFinalNewFileLine = 0;

	ContentFiles* contentFiles = NULL;

	NumberList oldFileLinesToHighlight;
	oldFileLinesToHighlight.list = calloc(1, sizeof(int));
	oldFileLinesToHighlight.size = 0;

	NumberList newFileLinesToHighlight;
	newFileLinesToHighlight.list = calloc(1, sizeof(int));
	newFileLinesToHighlight.size = 0;

	float totalOldLinesToRender = 0;
	float totalNewLinesToRender = 0;

	const char* dragFileMessage = "Drag a file here to compare it";
	const char* fileLoadedMessage = "File loaded!";
	Vector2 dragFileMessageSize = MeasureTextEx(robotoMonoFont, dragFileMessage, FONT_SIZE, 0);
	Vector2 fileLoadedMessageSize = MeasureTextEx(robotoMonoFont, fileLoadedMessage, FONT_SIZE, 0);

	int filePathCounter = 0;
	char* filePaths[MAX_FILEPATHS] = {0};

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

		DrawLine(WIDTH / 2, 0, WIDTH / 2, HEIGHT, WHITE);

		size_t width = WIDTH / 4;

#ifdef _DEBUG
		DrawFPS(0, 0);
#endif

		if (IsFileDropped())
		{
			FilePathList droppedFiles = LoadDroppedFiles();

			for (int i = 0, offset = filePathCounter; i < (int)droppedFiles.count; i++)
			{
				if (filePathCounter < 2)
				{
					if (filePaths[offset + i] != NULL) continue;

					filePaths[offset + i] = calloc(strlen(droppedFiles.paths[i]), sizeof(char));

					if (filePaths[offset + i] == NULL)
					{ 
						puts("Unable to allocate memory for file path.");
						exit(1);
					}

					TextCopy(filePaths[offset + i], droppedFiles.paths[i]);
					filePathCounter++;
				}
			}

			UnloadDroppedFiles(droppedFiles);

			if (filePathCounter > 0 && filePaths[0] != NULL && oldFileBuffer == NULL)
			{
				oldFileBuffer = readFileToBuffer(filePaths[0], &oldFileSizeInBytes);
			}

			if (filePathCounter > 1 && filePaths[1] != NULL && newFileBuffer == NULL)
			{
				newFileBuffer = readFileToBuffer(filePaths[1], &newFileSizeInBytes);
			}
		}

		if (oldFileBuffer == NULL)
		{
			DrawText(dragFileMessage, (WIDTH / 4) - (dragFileMessageSize.x / 2), (HEIGHT / 2) - FONT_SIZE / 2, FONT_SIZE, (Color) { .r = 255, .g = 255, .b = 255, .a = 255 });
		}
		else if (contentFiles == NULL)
		{
			DrawText(fileLoadedMessage, (WIDTH / 4) - (fileLoadedMessageSize.x / 2), (HEIGHT / 2) - FONT_SIZE / 2, FONT_SIZE, (Color) { .r = 255, .g = 255, .b = 255, .a = 255 });
		}

		if (newFileBuffer == NULL)
		{
			DrawText(dragFileMessage, (WIDTH - (WIDTH / 4)) - (dragFileMessageSize.x / 2), (HEIGHT / 2) - FONT_SIZE / 2, FONT_SIZE, (Color) { .r = 255, .g = 255, .b = 255, .a = 255 });
		}

		if (contentFiles != NULL)
		{
			renderOldFileLines(contentFiles, totalOldLinesToRender, &oldFileLinesToHighlight, &robotoMonoFont, &oldFileScrollIndex, &renderedFinalOldFileLine);
			renderNewFileLines(contentFiles, totalNewLinesToRender, &newFileLinesToHighlight, &robotoMonoFont, &newFileScrollIndex, &renderedFinalNewFileLine);

			setScrollIndexBasedOnMouseWheelMovement(&oldFileScrollIndex, &newFileScrollIndex, &renderedFinalOldFileLine, &renderedFinalNewFileLine);
		}
		else if (oldFileBuffer != NULL && newFileBuffer != NULL)
		{
			contentFiles = loadFilesIntoMemory(oldFileBuffer, newFileBuffer, oldFileSizeInBytes, newFileSizeInBytes);

			findChangedLinesInOldFile(&oldFileLinesToHighlight, contentFiles);
			findChangedLinesInNewFile(&newFileLinesToHighlight, contentFiles);

			if (contentFiles != NULL)
			{
				totalOldLinesToRender = getNumberOfLinesThatWillBeRendered(&contentFiles->oldFileLinesList.list, &robotoMonoFont);
				totalNewLinesToRender = getNumberOfLinesThatWillBeRendered(&contentFiles->newFileLinesList.list, &robotoMonoFont);
			}
		}

		EndDrawing();
	}

	// cleanup
	UnloadFont(robotoMonoFont);

	if (oldFileBuffer != 0)
		free(oldFileBuffer);

	if (newFileBuffer != 0)
		free(newFileBuffer);

	if (contentFiles != 0)
	{
		for (size_t i = 0; i < contentFiles->oldFileLinesList.size; i++)
		{
			free(contentFiles->oldFileLinesList.list[i]);
		}

		for (size_t i = 0; i < contentFiles->newFileLinesList.size; i++)
		{
			free(contentFiles->newFileLinesList.list[i]);
		}

		free(contentFiles);
	}

	CloseWindow();

	return EXIT_SUCCESS;
}
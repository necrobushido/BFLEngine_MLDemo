#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <iostream>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>
#include <limits.h>
#include <windows.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

const char*		kFbxExtension = "fbx";
const char*		kFbxAnimExtension = "fbxa";
const char*		kDescriptionFile = "Desc.txt";

int ExecuteCommand(char* cmd) 
{
	printf("%s\n", cmd);
			
    FILE*	pipe = _popen(cmd, "r");
    
	if( !pipe )
	{
		return 0;
	}

	char	textOutput[MAX_PATH * 10];
	textOutput[0] = '\0';
    char	buffer[MAX_PATH];
    while(!feof(pipe)) 
	{
    	if( fgets(buffer, 128, pipe) != NULL )
		{
			//strcat(textOutput, buffer);
		}
    }
    _pclose(pipe);

	//printf("%s", textOutput);
    return 1;
}

void RemoveExtension(const char* originalFilename, char* outputFilename)
{
	strcpy(outputFilename, originalFilename);

	char*	lastPeriod = strrchr(outputFilename, '.');
	lastPeriod[0] = '\0';
}

void ReplaceExtension(const char* originalFilename, char* outputFilename, const char* newExtension)
{
	strcpy(outputFilename, originalFilename);

	char*	lastPeriod = strrchr(outputFilename, '.');
	lastPeriod[1] = '\0';
	strcat(outputFilename, newExtension);
}

void PutInQuotes(const char* original, char* output)
{
	strcpy(output, "\"");
	strcat(output, original);
	strcat(output, "\"");
}

void CreateDir(const char* outputPath)
{
	CreateDirectory(outputPath, NULL);
}

FILETIME TimeFileModified(const char* filenameWithPath)
{
	FILETIME	returnValue;
	memset(&returnValue, 0, sizeof(FILETIME));

	HANDLE	fileHandle = CreateFile(filenameWithPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if( fileHandle == INVALID_HANDLE_VALUE )
	{
		return returnValue;
	}

	FILETIME	creationTime;
	FILETIME	writeTime;
	BOOL		timeFound = GetFileTime(fileHandle, &creationTime, NULL, &writeTime);
	if( timeFound )
	{
		LONG	firstIsOlderThanSecond = -1;
		//LONG	timesEqual = 0;
		//LONG	firstIsYoungerThanSecond = 1;
		if( CompareFileTime(&writeTime, &creationTime) == firstIsOlderThanSecond )
		{
			returnValue = creationTime;
		}
		else
		{
			returnValue = writeTime;
		}
	}

	CloseHandle(fileHandle);

	return returnValue;
}

bool NeedsToBeUpdated(const char* outputFilename, const char* inputFilename, const char* dependency)
{
	bool	returnValue = false;

	FILETIME	outputFiletime = TimeFileModified(outputFilename);
	if( outputFiletime.dwHighDateTime == 0 &&
		outputFiletime.dwLowDateTime == 0 )
	{
		//	the file doesn't exist
		returnValue = true;
	}
	else
	{
		FILETIME	inputFiletime = TimeFileModified(inputFilename);
		if( CompareFileTime(&outputFiletime, &inputFiletime) < 0 )
		{
			returnValue = true;
		}

		if( dependency )
		{
			FILETIME	dependFiletime = TimeFileModified(dependency);
			if( CompareFileTime(&outputFiletime, &dependFiletime) < 0 )
			{
				returnValue = true;
			}
		}
	}

	return returnValue;
}

void CopyInputFileToOutput(const char* inputPath, const char* outputPath)
{
	char		command[MAX_PATH];

	if( NeedsToBeUpdated(outputPath, inputPath, "cp.exe") )
	{
		strcpy(command, "cp.exe ");
		strcat(command, inputPath);
		strcat(command, " ");
		strcat(command, outputPath);

		int	success = ExecuteCommand(command);
		assert(success);
	}
}

void WriteTextToFile(const char* fileText, const char *filename)
{
	FILE*	pFile = fopen(filename, "wb");
	if( pFile == NULL )
	{
		printf("error opening file %s\n", filename);
		assert(0);
	}

	size_t	expectedBytesWritten = sizeof(char) * strlen(fileText);
	size_t	bytesWritten = fwrite(fileText, 1, expectedBytesWritten, pFile);
	assert(bytesWritten == expectedBytesWritten);

	fclose(pFile);
}

void BuildFile(const char* filename, const char* inputPath, const char* outputPath)
{
	char	temp[MAX_PATH];

	char	filenameWithoutExtension[MAX_PATH];
	RemoveExtension(filename, filenameWithoutExtension);

	char	filenameWithAnimExtension[MAX_PATH];
	ReplaceExtension(filename, filenameWithAnimExtension, kFbxAnimExtension);	

	//	create a subdirectory in the output path for the file
	char	thisFileOutputDir[MAX_PATH];
	strcpy(thisFileOutputDir, outputPath);
	strcat(thisFileOutputDir, "\\");
	strcat(thisFileOutputDir, filenameWithoutExtension);
	strcat(thisFileOutputDir, "\\");
	CreateDir(thisFileOutputDir);

	//	copy the source file over and rename it
	char	thisFileInputPath[MAX_PATH];	
	strcpy(thisFileInputPath, inputPath);
	strcat(thisFileInputPath, "\\");
	strcat(thisFileInputPath, filename);
	strcpy(temp, thisFileInputPath);
	PutInQuotes(temp, thisFileInputPath);

	char	thisFileOutputPath[MAX_PATH];
	strcpy(thisFileOutputPath, thisFileOutputDir);
	strcat(thisFileOutputPath, filenameWithAnimExtension);
	strcpy(temp, thisFileOutputPath);
	PutInQuotes(temp, thisFileOutputPath);

	CopyInputFileToOutput(thisFileInputPath, thisFileOutputPath);

	//	create a description file for the input file
	strcpy(thisFileOutputPath, thisFileOutputDir);
	strcat(thisFileOutputPath, kDescriptionFile);
	//strcpy(temp, thisFileOutputPath);
	//PutInQuotes(temp, thisFileOutputPath);
	WriteTextToFile(filenameWithoutExtension, thisFileOutputPath);
}

void BuildInputFiles(const char* inputPath, const char* outputPath)
{
	WIN32_FIND_DATA	findFileData;
	HANDLE			hFind;

	char	fileExpression[MAX_PATH];
	strcpy(fileExpression, inputPath);
	strcat(fileExpression, "\\");
	strcat(fileExpression, "*.");
	strcat(fileExpression, kFbxExtension);

	hFind = FindFirstFile(fileExpression, &findFileData);

	int		fileFound = hFind != INVALID_HANDLE_VALUE;
	while(fileFound)
	{
		BuildFile(findFileData.cFileName, inputPath, outputPath);

		fileFound = FindNextFile(hFind, &findFileData);
	}
}

int main()
{
	//	we want to take some input directory and create a new directory with the original's contents but sorted
	//		this sorting will involve creating a folder for each file, adding that file but renaming it, and adding a base description file

	const char*	kInputPath = "InputData";
	const char*	kOutputPath = "OutputData";

	char		inputPath[MAX_PATH];
	char		outputPath[MAX_PATH];

	strcpy(inputPath, kInputPath);
	strcpy(outputPath, kOutputPath);

	//	create the output directory
	CreateDir(outputPath);

	//	build the files in the input directory
	BuildInputFiles(inputPath, outputPath);

	return 0;
}
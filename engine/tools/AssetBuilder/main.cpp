#include "stdafx.h"

#include "BuildAssetThread.h"

const char*	kBaseInputPath = "..\\..\\..\\projects\\%s\\resources\\";
const char*	kBaseOutputPath = "..\\..\\..\\projects\\%s\\data\\";

bool		s_flatOutputDir = true;

struct SupportedExtension
{
	char	extension[PATH_BUFFER_SIZE];
	void	(*buildFunctionPtr)(const char* filename, const char* inputDir, const char* outputPath);
};

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
		DWORD	errorCode = GetLastError();
        char	errorMessage[PATH_BUFFER_SIZE];
        if( FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMessage, PATH_BUFFER_SIZE, NULL) )
		{
			//	beware the print spam from this on new checkouts
			//printf(errorMessage);
			//printf("\n");
		}

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

void BuildFBXModel(const char* filename, const char* inputPath, const char* outputPath)
{
	char		command[PATH_BUFFER_SIZE];
	char		outputFilename[PATH_BUFFER_SIZE];
	char		outputFilenameWithPath[PATH_BUFFER_SIZE];
	char		inputFilenameWithPath[PATH_BUFFER_SIZE];
	char		temp[PATH_BUFFER_SIZE];
	const char*	outputExtension = "mdl";

	strcpy(inputFilenameWithPath, inputPath);
	strcat(inputFilenameWithPath, filename);		

	ReplaceExtension(filename, outputFilename, outputExtension);
	strcpy(outputFilenameWithPath, outputPath);
	strcat(outputFilenameWithPath, outputFilename);

	if( NeedsToBeUpdated(outputFilenameWithPath, inputFilenameWithPath, "FBXConverter.exe") )
	{
		strcpy(temp, inputFilenameWithPath);
		PutInQuotes(temp, inputFilenameWithPath);

		strcpy(temp, outputFilenameWithPath);
		PutInQuotes(temp, outputFilenameWithPath);

		strcpy(command, "FBXConverter.exe -m ");
		strcat(command, inputFilenameWithPath);
		strcat(command, " ");
		strcat(command, outputFilenameWithPath);

		BuildAssetThread::ExecuteBuildCommand(command);
	}
}

void BuildFBXAnimation(const char* filename, const char* inputPath, const char* outputPath)
{
	char		command[PATH_BUFFER_SIZE];
	char		outputFilename[PATH_BUFFER_SIZE];
	char		outputFilenameWithPath[PATH_BUFFER_SIZE];
	char		inputFilenameWithPath[PATH_BUFFER_SIZE];
	char		temp[PATH_BUFFER_SIZE];
	const char*	outputExtension = "anm";

	strcpy(inputFilenameWithPath, inputPath);
	strcat(inputFilenameWithPath, filename);			

	ReplaceExtension(filename, outputFilename, outputExtension);
	strcpy(outputFilenameWithPath, outputPath);
	strcat(outputFilenameWithPath, outputFilename);

	if( NeedsToBeUpdated(outputFilenameWithPath, inputFilenameWithPath, "FBXConverter.exe") )
	{
		strcpy(temp, inputFilenameWithPath);
		PutInQuotes(temp, inputFilenameWithPath);

		strcpy(temp, outputFilenameWithPath);
		PutInQuotes(temp, outputFilenameWithPath);

		strcpy(command, "FBXConverter.exe -a ");
		strcat(command, inputFilenameWithPath);
		strcat(command, " ");
		strcat(command, outputFilenameWithPath);

		BuildAssetThread::ExecuteBuildCommand(command);
	}
}

void BuildFBX(const char* filename, const char* inputPath, const char* outputPath)
{
	BuildFBXModel(filename, inputPath, outputPath);
	BuildFBXAnimation(filename, inputPath, outputPath);
}

void CopyFile(const char* filename, const char* inputPath, const char* outputPath)
{
	char		temp[PATH_BUFFER_SIZE];
	char		command[PATH_BUFFER_SIZE];
	char		outputFilenameWithPath[PATH_BUFFER_SIZE];
	char		inputFilenameWithPath[PATH_BUFFER_SIZE];

	strcpy(inputFilenameWithPath, inputPath);
	strcat(inputFilenameWithPath, filename);

	strcpy(outputFilenameWithPath, outputPath);
	strcat(outputFilenameWithPath, filename);

	if( NeedsToBeUpdated(outputFilenameWithPath, inputFilenameWithPath, "cp.exe") )
	{
		strcpy(temp, inputFilenameWithPath);
		PutInQuotes(temp, inputFilenameWithPath);

		strcpy(temp, outputFilenameWithPath);
		PutInQuotes(temp, outputFilenameWithPath);

		strcpy(command, "cp.exe ");
		strcat(command, inputFilenameWithPath);
		strcat(command, " ");
		strcat(command, outputFilenameWithPath);

		BuildAssetThread::ExecuteBuildCommand(command);
	}
}

void BuildLVLX(const char* filename, const char* inputPath, const char* outputPath)
{
	char		command[PATH_BUFFER_SIZE];
	char		outputFilename[PATH_BUFFER_SIZE];
	char		outputFilenameWithPath[PATH_BUFFER_SIZE];
	char		inputFilenameWithPath[PATH_BUFFER_SIZE];
	const char*	outputExtension = "lvl";

	strcpy(inputFilenameWithPath, inputPath);
	strcat(inputFilenameWithPath, filename);		

	ReplaceExtension(filename, outputFilename, outputExtension);
	strcpy(outputFilenameWithPath, outputPath);
	strcat(outputFilenameWithPath, outputFilename);

	if( NeedsToBeUpdated(outputFilenameWithPath, inputFilenameWithPath, "LVLXConverter.exe") )
	{
		strcpy(command, "LVLXConverter.exe ");
		strcat(command, inputFilenameWithPath);
		strcat(command, " ");
		strcat(command, outputFilenameWithPath);

		BuildAssetThread::ExecuteBuildCommand(command);
	}
}

void BuildTTF(const char* filename, const char* inputPath, const char* outputPath)
{
	char		command[PATH_BUFFER_SIZE];
	char		outputFilename[PATH_BUFFER_SIZE];
	char		outputFilenameWithPath[PATH_BUFFER_SIZE];
	char		inputFilenameWithPath[PATH_BUFFER_SIZE];
	const char*	outputExtension = "font";

	strcpy(inputFilenameWithPath, inputPath);
	strcat(inputFilenameWithPath, filename);		

	ReplaceExtension(filename, outputFilename, outputExtension);
	strcpy(outputFilenameWithPath, outputPath);
	strcat(outputFilenameWithPath, outputFilename);

	if( NeedsToBeUpdated(outputFilenameWithPath, inputFilenameWithPath, "TTFConverter.exe") )
	{
		strcpy(command, "TTFConverter.exe ");
		strcat(command, inputFilenameWithPath);
		strcat(command, " ");
		strcat(command, outputFilenameWithPath);

		BuildAssetThread::ExecuteBuildCommand(command);
	}
}

const SupportedExtension	supportedExtensions[] =
{
	{ "fbxm",		BuildFBXModel },
	{ "fbxa",		BuildFBXAnimation },
	{ "daem",		BuildFBXModel },
	{ "daea",		BuildFBXAnimation },
	{ "tga",		CopyFile },
	{ "jpg",		CopyFile },
	{ "png",		CopyFile },
	{ "vertex",		CopyFile },
	{ "geometry",	CopyFile },
	{ "fragment",	CopyFile },
	{ "lvlx",		BuildLVLX },
	{ "ttf",		CopyFile },
	{ "otf",		CopyFile },
	{ "csv",		CopyFile },
	{ "txt",		CopyFile },
    { "wav",        CopyFile },
	{ "anm",        CopyFile },
	{ "mdl",        CopyFile }
};

const int	kNumSupportedExtensions = ARRAY_SIZE(supportedExtensions);

void BuildFilesRecurse(const char* inputPath, const char* outputPath)
{
	WIN32_FIND_DATA	findFileData;
	HANDLE			hFind;

	for(int i = 0; i < kNumSupportedExtensions; i++)
	{
		char	fileExpression[PATH_BUFFER_SIZE];
		strcpy(fileExpression, inputPath);
		strcat(fileExpression, "*.");
		strcat(fileExpression, supportedExtensions[i].extension);

		hFind = FindFirstFile(fileExpression, &findFileData);

		int	fileFound = hFind != INVALID_HANDLE_VALUE;
		while(fileFound)
		{
			supportedExtensions[i].buildFunctionPtr(findFileData.cFileName, inputPath, outputPath);

			fileFound = FindNextFile(hFind, &findFileData);
		}
	}

	//	handle subdirectories
	{
		char	fileExpression[PATH_BUFFER_SIZE];
		strcpy(fileExpression, inputPath);
		strcat(fileExpression, "*");
		hFind = FindFirstFile(fileExpression, &findFileData);

		int	fileFound = hFind != INVALID_HANDLE_VALUE;
		while(fileFound)
		{
			if( (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				strcmp(findFileData.cFileName, ".") != 0 &&
				strcmp(findFileData.cFileName, "..") != 0 )
			{
				//	found a subdirectory; recurse into it
				char	newInputDir[PATH_BUFFER_SIZE];
				strcpy(newInputDir, inputPath);
				strcat(newInputDir, findFileData.cFileName);
				strcat(newInputDir, "\\");

				char	newOutputDir[PATH_BUFFER_SIZE];
				strcpy(newOutputDir, outputPath);

				if( !s_flatOutputDir )
				{
					strcat(newOutputDir, findFileData.cFileName);
					strcat(newOutputDir, "\\");

					CreateDir(newOutputDir);
				}

				BuildFilesRecurse(newInputDir, newOutputDir);		
			}

			fileFound = FindNextFile(hFind, &findFileData);
		}
	}
}

void BuildHeaders(const char* inputPath)
{
	char		command[PATH_BUFFER_SIZE];
	char		outputFilename[PATH_BUFFER_SIZE];
	char		inputFilename[PATH_BUFFER_SIZE];
	const char*	kGameDir = "..\\game\\";

	strcpy(inputFilename, inputPath);
	strcat(inputFilename, "EntityTypes.xml");
	
	strcpy(outputFilename, inputPath);
	strcat(outputFilename, kGameDir);
	strcat(outputFilename, "EntityTypes.h");		

	if( NeedsToBeUpdated(outputFilename, inputFilename, "DotHGenerator.exe") )
	{	
		strcpy(command, "DotHGenerator.exe -makeentitytypes ");
		strcat(command, inputFilename);
		strcat(command, " ");
		strcat(command, outputFilename);

		BuildAssetThread::ExecuteBuildCommand(command);
	}

	strcpy(outputFilename, inputPath);
	strcat(outputFilename, kGameDir);
	strcat(outputFilename, "EntityRegistry.h");		

	if( NeedsToBeUpdated(outputFilename, inputFilename, "DotHGenerator.exe") )
	{	
		strcpy(command, "DotHGenerator.exe -makeentityregistry ");
		strcat(command, inputFilename);
		strcat(command, " ");
		strcat(command, outputFilename);

		BuildAssetThread::ExecuteBuildCommand(command);
	}

	strcpy(outputFilename, inputPath);
	strcat(outputFilename, kGameDir);
	strcat(outputFilename, "EntityIncludes.h");		

	if( NeedsToBeUpdated(outputFilename, inputFilename, "DotHGenerator.exe") )
	{	
		strcpy(command, "DotHGenerator.exe -makeentityincludes ");
		strcat(command, inputFilename);
		strcat(command, " ");
		strcat(command, outputFilename);

		BuildAssetThread::ExecuteBuildCommand(command);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	if( argc < 2 )
	{
		//printf("AssetBuilder : improper command line\n");
		printf("AssetBuilder usage : AssetBuilder.exe projectName [flatOutput:-t or -f]\n");
		return 0;
	}

	char	inputPath[PATH_BUFFER_SIZE];
	char	outputPath[PATH_BUFFER_SIZE];
	char*	projectName = argv[1];
	char*	flatOutput = argv[2];

	if( strcmp(flatOutput, "-f") == 0 )
	{
		s_flatOutputDir = false;
	}

	sprintf(inputPath, kBaseInputPath, projectName);
	sprintf(outputPath, kBaseOutputPath, projectName);

	//	create the data directory for the project
	CreateDir(outputPath);

	//	call DotHGenerator
	BuildHeaders(inputPath);

	//	build the files in the resources directory for project
	BuildFilesRecurse(inputPath, outputPath);	

	return 0;
}


#include "FileManager.h"
#include "Hash.h"

namespace
{
	enum
	{
		kNumHashBits = 6
	};
}

//	FIXME : find a better place for this
char *ToLower(const char *input, char *output)
{
	u8	i = 0;
	while(input[i])
	{
		if( input[i] > 0x40 && 
			input[i] < 0x5B )
		{
			output[i] = (char)(input[i] + 0x20);
		}
		else
		{
			output[i] = input[i];
		}
		i++;
	}
	output[i] = 0;

	return output;
}

void GetFileExtension(const char* filename, char* extensionBufferOut, u32 outBufferSize)
{
	size_t		filenameLen = strlen(filename);
	const char*	lastPeriod = strrchr(filename, '.');
	if( lastPeriod != NULL )
	{
		strncpy(extensionBufferOut, lastPeriod+1, outBufferSize);

		//	just in case
		extensionBufferOut[outBufferSize-1] = '\0';

		ToLower(extensionBufferOut, extensionBufferOut);
	}
	else
	{
		extensionBufferOut[0] = '\0';
	}
}

u32 FilenameHash(const char *filename)
{
	char	filename_lower[MAX_PATH];
	ToLower(filename, filename_lower);
	return JenkinsHash(filename_lower);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FileManager	*g_fileManager = NULL;

FileManager::FileManager(const char *prePath):
	m_hashTable(kNumHashBits)
{
	Assert(g_fileManager == NULL);
	g_fileManager = this;

	//	get the directory that the .exe is in
	char	filePath[MAX_PATH];
	u64		dirLen;
	GetModuleFileNameA(NULL, filePath, MAX_PATH);
	dirLen = strrchr(filePath, '\\') - filePath;
	strncpy(m_fileDir, filePath, dirLen);
	m_fileDir[dirLen] = '\0';

	if( prePath )
	{
		strcat(m_fileDir, prePath);
	}

	//	add "\\data\\"
	strcat(m_fileDir, "\\data\\");
}

FileManager::~FileManager()
{
	g_fileManager = NULL;
}

PlaceholderFileRef FileManager::MakeRef(const char *filename)
{
	// If the managed file doesn't exist yet, then create it.
	const u32	hash = FilenameHash(filename);
	ManagedFile	*pMF = GetManagedFile(hash);
	if( pMF == NULL )
	{
		pMF = CreateManagedFile(hash, filename);
	}

	// Return a reference to the file
	PlaceholderFileRef	returnValue(reinterpret_cast<u32 *>(pMF->GetData()));
	return returnValue;
}

void FileManager::AddRef(void *pFileData)
{
	// Bad things will happen if this is not a valid file pointer.
	ManagedFile	*pMF = GetManagedFileHeader(pFileData);
	++pMF->refCount;
}


void FileManager::Release(void *pFileData)
{
	// Bad things will happen if this is not a valid file pointer.
	if( pFileData != NULL ) 
	{
		ManagedFile	*pMF = GetManagedFileHeader(pFileData);
		ReleaseManagedFile(pMF);
	}
}

size_t FileManager::GetFileSize(void *pFileData)
{
	size_t	returnValue = 0;
	if( pFileData != NULL ) 
	{
		ManagedFile	*pMF = GetManagedFileHeader(pFileData);
		returnValue = pMF->fileSize;
	}

	return returnValue;
}

const char *FileManager::GetFilename(const void *pFileData)
{
	// Bad things will happen if this is not a valid file pointer.
	return (pFileData == NULL) ? "<null>" : GetManagedFileHeader(pFileData)->GetFilename();
}

bool FileManager::FileExists(const char *filename)
{
	bool	returnValue = true;
	char	fullFilename[MAX_PATH];
	strcpy(fullFilename, m_fileDir);
	strcat(fullFilename, filename);

	FILE	*pFile = fopen(fullFilename, "rb");
	if( pFile == NULL )
	{
		returnValue = false;
	}
	else
	{
		fclose(pFile);
	}	

	return returnValue;
}

bool FileManager::AbsoluteFileExists(const char *filename)
{
	bool	returnValue = true;

	FILE*	pFile = fopen(filename, "rb");
	if( pFile == NULL )
	{
		returnValue = false;
	}
	else
	{
		fclose(pFile);
	}	

	return returnValue;
}

FileManager::ManagedFile *FileManager::CreateManagedFile(u32 hash, const char *filename)
{
	char	fullFilename[MAX_PATH];
	strcpy(fullFilename, m_fileDir);
	strcat(fullFilename, filename);

	FILE	*pFile = fopen(fullFilename, "rb");
	if( pFile == NULL )
	{
		//	file not found?
		_Panicf("file \"%s\" not found!\n", filename);
	}

	fseek(pFile, 0, SEEK_END);

	size_t	fileSize = ftell(pFile);

	size_t	paddedFileSize = fileSize;

	paddedFileSize = (paddedFileSize + 0x1f) & ~0x1f;	//	round up to multiple of 32

	// Allocated space for the managed file and fill in the header.
	// Leave room for the filename plus 2 more bytes - one for the
	// filename null terminator and one for another copy of the
	// headerSize (in cache lines).  This allows for obtaining a
	// ManagedFile pointer from a filedata pointer.
	const size_t	headerSize = ((sizeof(ManagedFile) + (strlen(filename) + 2)) + 0x1f) & ~0x1f;
	ManagedFile		*pMF = reinterpret_cast<ManagedFile *>(new char[headerSize + paddedFileSize]);
	pMF->hash = hash;
	pMF->refCount = 0;
	pMF->fileSize = fileSize;
	pMF->numHeaderCacheLines = u8(headerSize >> 5);
	strcpy(reinterpret_cast<char *>(pMF + 1), filename);
	reinterpret_cast<char *>(pMF)[headerSize-1] = (char)(headerSize >> 5);

	rewind(pFile);
	size_t	readSize = fread(pMF->GetData(), 1, fileSize, pFile);
	if( readSize != fileSize )
	{
		if( ferror(pFile) )
		{
			DebugPrintf("error reading from file\n");
		}
		else
		if( feof(pFile) )
		{
			DebugPrintf("reached end of file\n");
		}
	}
	Assert(readSize == fileSize);
	fclose(pFile);

	// Add the ManagedFile record to the list of files at the hash index for this file.
	m_hashTable.AddEntry(pMF, hash);

	// Return the file data
	return pMF;
}

FileManager::ManagedFile *FileManager::GetManagedFile(u32 hash)
{
	return m_hashTable.GetEntry(hash);
}

void FileManager::ReleaseManagedFile(ManagedFile *pMF)
{
	// Decrement our reference count.  It must reach zero before we can free it.
	if(	(pMF->refCount > 0) && 
		(--pMF->refCount == 0) )
	{
		// Free the asset.
		m_hashTable.RemoveEntry(pMF, pMF->hash);
		delete pMF;
	}
}

FileManager::ManagedFile *FileManager::GetManagedFileHeader(void *pFileData)
{
	return reinterpret_cast<ManagedFile *>(reinterpret_cast<char *>(pFileData) - (unsigned(*(reinterpret_cast<u8 *>(pFileData) - 1)) << 5));
}

const FileManager::ManagedFile *FileManager::GetManagedFileHeader(const void *pFileData)
{
	return reinterpret_cast<const ManagedFile *>(reinterpret_cast<const char *>(pFileData) - (unsigned(*(reinterpret_cast<const u8 *>(pFileData) - 1)) << 5));
}

void FileManager::WriteDataToFile(const char* filename, const void* data, int dataSize)
{
	const char*	dataDir = GetDataDir();

	char		fullFilePath[MAX_PATH];
	strcpy(fullFilePath, dataDir);
	strcat(fullFilePath, filename);

	FILE*		pFile = fopen(fullFilePath, "wb");
	if( pFile == NULL )
	{
		AssertMsg(0, "error opening file %s for writing\n", fullFilePath);
	}

	size_t	bytesWritten = fwrite(data, 1, dataSize, pFile);
	Assert(bytesWritten == dataSize);

	fclose(pFile);
}
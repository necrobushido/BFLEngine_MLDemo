#pragma once

#include "types.h"
#include "HashTable.h"

template<class = u32> class FileRef;

class PlaceholderFileRef;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GetFileExtension(const char* filename, char* extensionBufferOut, u32 outBufferSize);
char *ToLower(const char *input, char *output);
u32 FilenameHash(const char *filename);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FileManager
{
private:
	class ManagedFile : public HashTable<ManagedFile>::EntryMember
	{
	public:
		const char *GetFilename() const { return reinterpret_cast<const char *>(this + 1); }
		char *GetData() { return reinterpret_cast<char *>(this) + (unsigned(numHeaderCacheLines) << 5); }

	public:
		u32		hash;
		u32		refCount;
		size_t	fileSize;
		u8		numHeaderCacheLines;
	};

public:
	FileManager(const char *prePath = NULL);
	~FileManager();

public:
	PlaceholderFileRef MakeRef(const char *filename);
	void AddRef(void *pFileData);
	void Release(void *pFileData);
	size_t GetFileSize(void *pFileData);
	const char *GetFilename(const void *pFileData);
	bool FileExists(const char *filename);
	bool AbsoluteFileExists(const char *filename);
	const char* GetDataDir(){ return m_fileDir; }
	void WriteDataToFile(const char* filename, const void* data, int dataSize);

private:
	ManagedFile *CreateManagedFile(u32 hash, const char *filename);
	ManagedFile *GetManagedFile(u32 hash);
	void ReleaseManagedFile(ManagedFile *pMF);
	ManagedFile *GetManagedFileHeader(void *pFileData);
	const ManagedFile *GetManagedFileHeader(const void *pFileData);

private:
	HashTable<ManagedFile>	m_hashTable;
	char					m_fileDir[MAX_PATH];
};

extern FileManager	*g_fileManager;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T> class FileRef 
{
public:	
	// The constructors & destructor automatically manage the refcount
	// of the file.  Note that uncasted copy from PlaceholderFileRef is
	// always allowed regardless of the type of T.
	// this is to get an easy to use templated FileRef out of FileManager::MakeRef
	FileRef();
	FileRef(const char *filename);
	FileRef(const FileRef &ref);
	FileRef(const PlaceholderFileRef &ref);
	FileRef(T *pFileData);
	~FileRef();

public:	
	// Assignment.  Note that uncasted assignment from FileRef<u32> is
	// always allowed regardless of the type of T.
	FileRef &operator=(const FileRef &ref);
	FileRef &operator=(const PlaceholderFileRef &ref);
	FileRef &operator=(T *pFileData);

	// Equality
	bool operator==(const FileRef &ref) const;
	bool operator!=(const FileRef &ref) const;
	bool operator==(const T *pFileData) const;
	bool operator!=(const T *pFileData) const;

	// Sorting
	bool operator<(const FileRef &ref) const;
	bool operator>(const FileRef &ref) const;

	// Dereferencing
	T *operator->() const;
	T *operator*() const;
	
	// Casting operators.  These allow a FileRef<T> to be passed
	// by value into a function expecting a T* or a T&.
	operator T*();
	operator const T*() const;
	operator T&();
	operator const T&() const;

	// The filename of the file who's data we are referencing,
	// "<null>" if we are a null FileRef.
	const char *GetFilename() const;

	// The file hash of the file who's data we are referencing
	// (0 if we are a null FileRef).
	u32 GetHash() const;

	//
	size_t FileSize() const;
	
private:	
	T *m_ptr;
};

//	this only exists for ease of use / compile problems with FileManager::MakeRef and FileRef<u32>
class PlaceholderFileRef : public FileRef<u32>
{
public:
	PlaceholderFileRef(u32 *pFileData):
		FileRef<u32>(pFileData)
	{
	}		
};

#include "FileManager.inl"


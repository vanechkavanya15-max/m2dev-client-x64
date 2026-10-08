#include "StdAfx.h"

#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include <assert.h>
#include <sys/stat.h>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <string_view>
#include <optional>
#include <span>

#include <utf8.h>

#include "Utils.h"
#include "Stl.h"

// ============================================================================
// Helper utilities for UTF-8 <-> std::filesystem::path
// ============================================================================

static inline std::filesystem::path PathFromUtf8(std::string_view utf8Str)
{
	if (utf8Str.empty())
		return {};
	return std::filesystem::path(Utf8ToWide(std::string(utf8Str).c_str()));
}

static inline std::string PathToUtf8(const std::filesystem::path& p)
{
	return WideToUtf8(p.c_str());
}

// ============================================================================
// Temp file creation
// ============================================================================

const char* CreateTempFileName(const char* c_pszPrefix)
{
	thread_local std::string s_utf8TempName;

	wchar_t wTempPath[MAX_PATH + 1]{};
	wchar_t wTempName[MAX_PATH + 1]{};

	if (!GetTempPathW(MAX_PATH, wTempPath))
		return "";

	wchar_t wPrefix[4] = L"etb";
	if (c_pszPrefix && *c_pszPrefix)
	{
		std::wstring wp = Utf8ToWide(c_pszPrefix);
		wcsncpy_s(wPrefix, wp.c_str(), 3);
	}

	if (!GetTempFileNameW(wTempPath, wPrefix, 0, wTempName))
		return "";

	s_utf8TempName = WideToUtf8(wTempName);
	return s_utf8TempName.c_str();
}

std::string CreateTempFileName(std::string_view prefix)
{
	wchar_t wTempPath[MAX_PATH + 1]{};
	wchar_t wTempName[MAX_PATH + 1]{};

	if (!GetTempPathW(MAX_PATH, wTempPath))
		return {};

	wchar_t wPrefix[4] = L"etb";
	if (!prefix.empty())
	{
		std::string s(prefix);
		std::wstring wp = Utf8ToWide(s.c_str());
		wcsncpy_s(wPrefix, wp.c_str(), 3);
	}

	if (!GetTempFileNameW(wTempPath, wPrefix, 0, wTempName))
		return {};

	return WideToUtf8(wTempName);
}

std::filesystem::path CreateTempFilePath(std::string_view prefix)
{
	std::string name = CreateTempFileName(prefix);
	return PathFromUtf8(name);
}

// ============================================================================
// File Path & Extension Splitting
// ============================================================================

void GetFilePathNameExtension(std::string_view file, std::string* pstPath, std::string* pstName, std::string* pstExt)
{
	assert(pstPath != nullptr);
	assert(pstName != nullptr);
	assert(pstExt != nullptr);

	if (!pstPath || !pstName || !pstExt)
		return;

	size_t len = file.length();
	size_t ext = len;
	size_t pos = len;

	while (pos > 0)
	{
		--pos;
		char c = file[pos];
		if (ext == len && c == '.')
		{
			ext = pos;
			break;
		}
		if (c == '/' || c == '\\')
			break;
	}

	while (pos > 0)
	{
		--pos;
		char c = file[pos];
		if (c == '/' || c == '\\')
			break;
	}

	if (pos > 0)
	{
		++pos;
		pstPath->append(file.data(), pos);
	}

	if (ext > pos)
		pstName->append(file.data() + pos, ext - pos);

	++ext;
	if (len > ext)
		pstExt->append(file.data() + ext, len - ext);
}

void GetFilePathNameExtension(std::string_view file, std::string& rstPath, std::string& rstName, std::string& rstExt)
{
	rstPath.clear();
	rstName.clear();
	rstExt.clear();
	GetFilePathNameExtension(file, &rstPath, &rstName, &rstExt);
}

FileNameParts GetFilePathNameExtension(std::string_view file)
{
	FileNameParts parts;
	GetFilePathNameExtension(file, &parts.path, &parts.name, &parts.ext);
	return parts;
}

void GetFilePathNameExtension(const std::filesystem::path& path, std::string* pstPath, std::string* pstName, std::string* pstExt)
{
	std::string u8 = PathToUtf8(path);
	GetFilePathNameExtension(std::string_view(u8), pstPath, pstName, pstExt);
}

void GetFilePathNameExtension(const std::filesystem::path& path, std::string& rstPath, std::string& rstName, std::string& rstExt)
{
	rstPath.clear();
	rstName.clear();
	rstExt.clear();
	GetFilePathNameExtension(path, &rstPath, &rstName, &rstExt);
}

FileNameParts GetFilePathNameExtension(const std::filesystem::path& path)
{
	return GetFilePathNameExtension(std::string_view(PathToUtf8(path)));
}

void GetFilePathNameExtension(const char * c_szFile, int len, std::string * pstPath, std::string * pstName, std::string * pstExt)
{
	if (!c_szFile || len <= 0)
		return;
	GetFilePathNameExtension(std::string_view(c_szFile, static_cast<size_t>(len)), pstPath, pstName, pstExt);
}

// ============================================================================
// File Extension Extraction
// ============================================================================

void GetFileExtension(std::string_view file, std::string* pstExt)
{
	assert(pstExt != nullptr);
	if (!pstExt)
		return;

	size_t len = file.length();
	size_t ext = len;
	size_t pos = len;

	while (pos > 0)
	{
		--pos;
		char c = file[pos];
		if (ext == len && c == '.')
		{
			ext = pos;
			break;
		}
		if (c == '/' || c == '\\')
			break;
	}

	++ext;
	if (len > ext)
		pstExt->append(file.data() + ext, len - ext);
}

void GetFileExtension(std::string_view file, std::string& rstExt)
{
	rstExt.clear();
	GetFileExtension(file, &rstExt);
}

std::string GetFileExtension(std::string_view file)
{
	std::string ext;
	GetFileExtension(file, &ext);
	return ext;
}

void GetFileExtension(const std::filesystem::path& path, std::string* pstExt)
{
	if (!pstExt)
		return;
	std::string ext = PathToUtf8(path.extension());
	if (!ext.empty() && ext[0] == '.')
		ext.erase(0, 1);
	*pstExt = std::move(ext);
}

void GetFileExtension(const std::filesystem::path& path, std::string& rstExt)
{
	rstExt = GetFileExtension(path);
}

std::string GetFileExtension(const std::filesystem::path& path)
{
	std::string ext = PathToUtf8(path.extension());
	if (!ext.empty() && ext[0] == '.')
		ext.erase(0, 1);
	return ext;
}

void GetFileExtension(const char* c_szFile, int len, std::string* pstExt)
{
	if (!c_szFile || len <= 0)
		return;
	GetFileExtension(std::string_view(c_szFile, static_cast<size_t>(len)), pstExt);
}

// ============================================================================
// File Name Parts
// ============================================================================

void GetFileNameParts(std::string_view file, std::string& rPath, std::string& rName, std::string& rExt)
{
	GetFilePathNameExtension(file, rPath, rName, rExt);
}

FileNameParts GetFileNameParts(std::string_view file)
{
	return GetFilePathNameExtension(file);
}

void GetFileNameParts(const std::filesystem::path& path, std::string& rPath, std::string& rName, std::string& rExt)
{
	GetFilePathNameExtension(path, rPath, rName, rExt);
}

FileNameParts GetFileNameParts(const std::filesystem::path& path)
{
	return GetFilePathNameExtension(path);
}

void GetFileNameParts(const char* c_szFile, size_t fileLen, char* pszPath, size_t pathLen, char* pszName, size_t nameLen, char* pszExt, size_t extLen)
{
	if (!c_szFile)
		return;

	FileNameParts parts = GetFilePathNameExtension(std::string_view(c_szFile, fileLen));
	if (pszPath && pathLen > 0)
	{
		size_t toCopy = std::min(parts.path.length(), pathLen - 1);
		memcpy(pszPath, parts.path.data(), toCopy);
		pszPath[toCopy] = '\0';
	}
	if (pszName && nameLen > 0)
	{
		size_t toCopy = std::min(parts.name.length(), nameLen - 1);
		memcpy(pszName, parts.name.data(), toCopy);
		pszName[toCopy] = '\0';
	}
	if (pszExt && extLen > 0)
	{
		size_t toCopy = std::min(parts.ext.length(), extLen - 1);
		memcpy(pszExt, parts.ext.data(), toCopy);
		pszExt[toCopy] = '\0';
	}
}

void GetFileNameParts(const char* c_szFile, int len, char* pszPath, char* pszName, char* pszExt)
{
	assert(pszPath != nullptr);
	assert(pszName != nullptr);
	assert(pszExt != nullptr);

	if (!c_szFile || len <= 0)
	{
		if (pszPath) *pszPath = '\0';
		if (pszName) *pszName = '\0';
		if (pszExt) *pszExt = '\0';
		return;
	}

	FileNameParts parts = GetFilePathNameExtension(std::string_view(c_szFile, static_cast<size_t>(len)));
	if (pszPath)
	{
		memcpy(pszPath, parts.path.data(), parts.path.length());
		pszPath[parts.path.length()] = '\0';
	}
	if (pszName)
	{
		memcpy(pszName, parts.name.data(), parts.name.length());
		pszName[parts.name.length()] = '\0';
	}
	if (pszExt)
	{
		memcpy(pszExt, parts.ext.data(), parts.ext.length());
		pszExt[parts.ext.length()] = '\0';
	}
}

// ============================================================================
// Indexing Names
// ============================================================================

void GetOldIndexingName(std::string& rName, int Index)
{
	rName += std::to_string(Index);
}

std::string GetOldIndexingName(int Index)
{
	return std::to_string(Index);
}

void GetOldIndexingName(char * szName, int Index)
{
	if (!szName)
		return;
	std::string s = std::to_string(Index);
	strcat(szName, s.c_str());
}

void GetOldIndexingName(char * szName, size_t maxLen, int Index)
{
	if (!szName || maxLen == 0)
		return;
	std::string s = std::to_string(Index);
	strncat_s(szName, maxLen, s.c_str(), _TRUNCATE);
}

void GetIndexingName(std::string& rName, DWORD Index)
{
	rName += std::to_string(Index);
}

std::string GetIndexingName(DWORD Index)
{
	return std::to_string(Index);
}

void GetIndexingName(char * szName, DWORD Index)
{
	if (!szName)
		return;
	std::string s = std::to_string(Index);
	strcat(szName, s.c_str());
}

void GetIndexingName(char * szName, size_t maxLen, DWORD Index)
{
	if (!szName || maxLen == 0)
		return;
	std::string s = std::to_string(Index);
	strncat_s(szName, maxLen, s.c_str(), _TRUNCATE);
}

// ============================================================================
// Case Conversion Helpers
// ============================================================================

std::string stl_lowers(std::string_view str)
{
	std::string res(str);
	stl_lowers(res);
	return res;
}

// ============================================================================
// Only Filename Extraction
// ============================================================================

void GetOnlyFileName(std::string_view name, std::string & strFileName)
{
	size_t lastSlash = name.find_last_of("/\\");
	if (lastSlash != std::string_view::npos)
		strFileName = std::string(name.substr(lastSlash + 1));
	else
		strFileName = std::string(name);
}

void GetOnlyFileName(const char * sz_Name, std::string & strFileName)
{
	if (!sz_Name)
	{
		strFileName.clear();
		return;
	}
	GetOnlyFileName(std::string_view(sz_Name), strFileName);
}

void GetOnlyFileName(const std::filesystem::path& path, std::string & strFileName)
{
	strFileName = PathToUtf8(path.filename());
}

std::string GetOnlyFileName(std::string_view name)
{
	std::string res;
	GetOnlyFileName(name, res);
	return res;
}

std::string GetOnlyFileName(const std::filesystem::path& path)
{
	return PathToUtf8(path.filename());
}

// ============================================================================
// Exception Path Name
// ============================================================================

void GetExceptionPathName(const char * sz_Name, std::string & OnlyFileName)
{
	GetOnlyFileName(sz_Name, OnlyFileName);
}

void GetExceptionPathName(std::string_view name, std::string & OnlyFileName)
{
	GetOnlyFileName(name, OnlyFileName);
}

void GetExceptionPathName(const std::filesystem::path& path, std::string & OnlyFileName)
{
	GetOnlyFileName(path, OnlyFileName);
}

std::string GetExceptionPathName(std::string_view name)
{
	return GetOnlyFileName(name);
}

std::string GetExceptionPathName(const std::filesystem::path& path)
{
	return GetOnlyFileName(path);
}

// ============================================================================
// Only Pathname Extraction
// ============================================================================

void GetOnlyPathName(std::string_view name, std::string & OnlyPathName)
{
	size_t lastSlash = name.find_last_of("/\\");
	if (lastSlash != std::string_view::npos)
		OnlyPathName = std::string(name.substr(0, lastSlash + 1));
	else
		OnlyPathName.clear();
}

void GetOnlyPathName(const char * sz_Name, std::string & OnlyPathName)
{
	if (!sz_Name)
	{
		OnlyPathName.clear();
		return;
	}
	GetOnlyPathName(std::string_view(sz_Name), OnlyPathName);
}

void GetOnlyPathName(const std::filesystem::path& path, std::string & OnlyPathName)
{
	std::string s = PathToUtf8(path.parent_path());
	if (!s.empty())
		s += '/';
	OnlyPathName = std::move(s);
}

std::string GetOnlyPathName(std::string_view name)
{
	std::string res;
	GetOnlyPathName(name, res);
	return res;
}

std::string GetOnlyPathName(const std::filesystem::path& path)
{
	std::string res;
	GetOnlyPathName(path, res);
	return res;
}

const char * GetOnlyPathName(const char * c_szName)
{
	thread_local std::string strPathName;
	GetOnlyPathName(c_szName, strPathName);
	return strPathName.c_str();
}

// ============================================================================
// Local File Name
// ============================================================================

bool GetLocalFileName(std::string_view globalPath, std::string_view fullPathFileName, std::string * pstrLocalFileName)
{
	if (!pstrLocalFileName)
		return false;

	std::string normGlobal = StringPath(globalPath);
	std::string normFull = StringPath(fullPathFileName);

	if (normGlobal.length() >= normFull.length())
		return false;

	if (normFull.compare(0, normGlobal.length(), normGlobal) != 0)
		return false;

	pstrLocalFileName->assign(fullPathFileName.substr(normGlobal.length()));
	return true;
}

bool GetLocalFileName(std::string_view globalPath, std::string_view fullPathFileName, std::string& rstrLocalFileName)
{
	return GetLocalFileName(globalPath, fullPathFileName, &rstrLocalFileName);
}

bool GetLocalFileName(const char * c_szGlobalPath, const char * c_szFullPathFileName, std::string * pstrLocalFileName)
{
	if (!c_szGlobalPath || !c_szFullPathFileName)
		return false;
	return GetLocalFileName(std::string_view(c_szGlobalPath), std::string_view(c_szFullPathFileName), pstrLocalFileName);
}

bool GetLocalFileName(const std::filesystem::path& globalPath, const std::filesystem::path& fullPathFileName, std::filesystem::path& rLocalFileName)
{
	std::string local;
	if (GetLocalFileName(std::string_view(PathToUtf8(globalPath)), std::string_view(PathToUtf8(fullPathFileName)), local))
	{
		rLocalFileName = PathFromUtf8(local);
		return true;
	}
	return false;
}

std::optional<std::string> GetLocalFileName(std::string_view globalPath, std::string_view fullPathFileName)
{
	std::string local;
	if (GetLocalFileName(globalPath, fullPathFileName, local))
		return local;
	return std::nullopt;
}

std::optional<std::filesystem::path> GetLocalFileName(const std::filesystem::path& globalPath, const std::filesystem::path& fullPathFileName)
{
	std::filesystem::path local;
	if (GetLocalFileName(globalPath, fullPathFileName, local))
		return local;
	return std::nullopt;
}

// ============================================================================
// Working Folder
// ============================================================================

void GetWorkingFolder(std::string & strFileName)
{
	wchar_t wbuf[MAX_PATH + 1]{};
	if (_wgetcwd(wbuf, MAX_PATH))
	{
		strFileName = WideToUtf8(wbuf);
		StringPath(strFileName);
		if (strFileName.empty() || strFileName.back() != '/')
			strFileName += '/';
	}
	else
	{
		strFileName = "./";
	}
}

std::string GetWorkingFolder()
{
	std::string folder;
	GetWorkingFolder(folder);
	return folder;
}

std::filesystem::path GetWorkingFolderPath()
{
	std::error_code ec;
	return std::filesystem::current_path(ec);
}

// ============================================================================
// String Lowers
// ============================================================================

void StringLowers(char * pString)
{
	if (!pString)
		return;
	while (*pString)
	{
		*pString = ascii_tolower(*pString);
		++pString;
	}
}

void StringLowers(char * pString, size_t maxLen)
{
	if (!pString)
		return;
	for (size_t i = 0; i < maxLen && pString[i]; ++i)
		pString[i] = ascii_tolower(pString[i]);
}

void StringLowers(std::string & rString)
{
	stl_lowers(rString);
}

std::string StringLowers(std::string_view str)
{
	return stl_lowers(str);
}

// ============================================================================
// String Path Normalization
// ============================================================================

void StringPath(std::string & rString)
{
	for (char& c : rString)
	{
		if (c == '\\')
			c = '/';
		else
			c = ascii_tolower(c);
	}
}

void StringPath(char * pString)
{
	if (!pString)
		return;
	while (*pString)
	{
		if (*pString == '\\')
			*pString = '/';
		else
			*pString = ascii_tolower(*pString);
		++pString;
	}
}

void StringPath(const char * c_szSrc, char * szDest)
{
	if (!c_szSrc || !szDest)
		return;
	size_t len = strlen(c_szSrc);
	for (size_t i = 0; i < len; ++i)
	{
		if (c_szSrc[i] == '\\')
			szDest[i] = '/';
		else
			szDest[i] = ascii_tolower(c_szSrc[i]);
	}
	szDest[len] = '\0';
}

void StringPath(const char * c_szSrc, char * szDest, size_t destLen)
{
	if (!c_szSrc || !szDest || destLen == 0)
		return;
	size_t len = strlen(c_szSrc);
	size_t toCopy = std::min(len, destLen - 1);
	for (size_t i = 0; i < toCopy; ++i)
	{
		if (c_szSrc[i] == '\\')
			szDest[i] = '/';
		else
			szDest[i] = ascii_tolower(c_szSrc[i]);
	}
	szDest[toCopy] = '\0';
}

void StringPath(const char * c_szSrc, std::string & rString)
{
	if (!c_szSrc)
	{
		rString.clear();
		return;
	}
	StringPath(std::string_view(c_szSrc), rString);
}

void StringPath(std::string_view src, std::string & rString)
{
	rString.resize(src.length());
	for (size_t i = 0; i < src.length(); ++i)
	{
		char c = src[i];
		rString[i] = (c == '\\') ? '/' : ascii_tolower(c);
	}
}

std::string StringPath(std::string_view src)
{
	std::string res;
	StringPath(src, res);
	return res;
}

std::filesystem::path StringPath(const std::filesystem::path& path)
{
	std::string s = PathToUtf8(path);
	StringPath(s);
	return PathFromUtf8(s);
}

// ============================================================================
// ASCII Data Dump
// ============================================================================

static inline bool ishprint(unsigned char x) noexcept
{
	return (((x & 0xE0) > 0x90) || isprint(x));
}

void PrintAsciiData(const void* void_data, int bytes)
{
	if (!void_data || bytes <= 0)
		return;

	const unsigned char* data = static_cast<const unsigned char*>(void_data);

	fprintf(stdout, "------------------------------------------------------------------\n");
	int j = bytes;
	while (j > 0)
	{
		int k = (j >= 16) ? 16 : j;

		const unsigned char* p = data;
		for (int i = 0; i < 16; ++i)
		{
			if (i >= k)
				fprintf(stdout, "   ");
			else
				fprintf(stdout, "%02x ", *p);
			p++;
		}

		fprintf(stdout, "| ");

		p = data;
		for (int i = 0; i < k; ++i)
		{
			if (i >= k)
				fprintf(stdout, " ");
			else
				fprintf(stdout, "%c", ishprint(*p) ? *p : '.');
			p++;
		}

		fprintf(stdout, "\n");

		j -= 16;
		data += 16;
	}

	fprintf(stdout, "------------------------------------------------------------------\n");
}

void PrintAsciiData(std::span<const uint8_t> data)
{
	PrintAsciiData(data.data(), static_cast<int>(data.size()));
}

void PrintAsciiData(std::string_view data)
{
	PrintAsciiData(data.data(), static_cast<int>(data.size()));
}

// ============================================================================
// Math Helpers
// ============================================================================

int MIN(int a, int b)
{
	return (a < b) ? a : b;
}

int MAX(int a, int b)
{
	return (a > b) ? a : b;
}

int MINMAX(int min, int value, int max)
{
	if (max < min)
		return MAX(min, value);

	return (value < min) ? min : ((value > max) ? max : value);
}

float fMIN(float a, float b)
{
	return (a < b) ? a : b;
}

float fMAX(float a, float b)
{
	return (a > b) ? a : b;
}

float fMINMAX(float min, float value, float max)
{
	if (max < min)
		return fMAX(min, value);

	return (value < min) ? min : ((value > max) ? max : value);
}

// ============================================================================
// File Checks
// ============================================================================

bool IsFile(const char* filename)
{
	if (!filename || !*filename)
		return false;
	return _access(filename, 0) == 0;
}

bool IsFile(std::string_view filename)
{
	if (filename.empty())
		return false;
	std::string s(filename);
	return _access(s.c_str(), 0) == 0;
}

bool IsFile(const std::filesystem::path& path)
{
	std::error_code ec;
	return std::filesystem::is_regular_file(path, ec);
}

bool IsGlobalFileName(const char * c_szFileName)
{
	if (!c_szFileName)
		return false;
	return strchr(c_szFileName, ':') != nullptr;
}

bool IsGlobalFileName(std::string_view filename)
{
	return filename.find(':') != std::string_view::npos;
}

bool IsGlobalFileName(const std::filesystem::path& path)
{
	return path.is_absolute() || path.has_root_name();
}

// ============================================================================
// Directory Creation & Removal
// ============================================================================

bool MyCreateDirectory(const std::filesystem::path& path)
{
	if (path.empty())
		return false;
	std::error_code ec;
	return std::filesystem::create_directories(path, ec) || !ec;
}

bool MyCreateDirectory(std::string_view pathUtf8)
{
	if (pathUtf8.empty())
		return false;
	return MyCreateDirectory(PathFromUtf8(pathUtf8));
}

void MyCreateDirectory(const char* pathUtf8)
{
	if (!pathUtf8 || !*pathUtf8)
		return;
	MyCreateDirectory(std::string_view(pathUtf8));
}

void RemoveAllDirectory(const std::filesystem::path& path)
{
	std::error_code ec;
	if (!std::filesystem::exists(path, ec))
		return;

	for (auto it = std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied, ec);
		 it != std::filesystem::recursive_directory_iterator(); ++it)
	{
		std::filesystem::permissions(it->path(), std::filesystem::perms::all, std::filesystem::perm_options::add, ec);
		::SetFileAttributesW(it->path().c_str(), FILE_ATTRIBUTE_NORMAL);
	}

	std::filesystem::remove_all(path, ec);
}

void RemoveAllDirectory(std::string_view directoryName)
{
	if (directoryName.empty())
		return;
	RemoveAllDirectory(PathFromUtf8(directoryName));
}

void RemoveAllDirectory(const char* c_szDirectoryName)
{
	if (!c_szDirectoryName || !*c_szDirectoryName)
		return;
	RemoveAllDirectory(std::string_view(c_szDirectoryName));
}

// ============================================================================
// String Except Character
// ============================================================================

void StringExceptCharacter(std::string * pstrString, const char * c_szCharacter)
{
	if (!pstrString || !c_szCharacter)
		return;
	StringExceptCharacter(*pstrString, std::string_view(c_szCharacter));
}

void StringExceptCharacter(std::string & rString, std::string_view characters)
{
	rString.erase(
		std::remove_if(rString.begin(), rString.end(),
			[&characters](char c) {
				return characters.find(c) != std::string_view::npos;
			}),
		rString.end());
}

std::string StringExceptCharacter(std::string_view str, std::string_view characters)
{
	std::string res(str);
	StringExceptCharacter(res, characters);
	return res;
}

// ============================================================================
// Split Line
// ============================================================================

bool SplitLine(std::string_view line, std::string_view delimiter, std::vector<std::string> * pkVec_strToken)
{
	if (!pkVec_strToken)
		return false;

	pkVec_strToken->clear();
	pkVec_strToken->reserve(10);

	if (line.empty())
		return false;

	size_t basePos = 0;

	do
	{
		size_t beginPos = line.find_first_not_of(delimiter, basePos);
		if (beginPos == std::string_view::npos)
			break;

		size_t endPos;

		if (line[beginPos] == '"')
		{
			++beginPos;
			endPos = line.find_first_of('"', beginPos);

			if (endPos == std::string_view::npos)
				return false;

			basePos = endPos + 1;
		}
		else
		{
			endPos = line.find_first_of(delimiter, beginPos);
			if (endPos == std::string_view::npos)
			{
				endPos = line.length();
				basePos = endPos;
			}
			else
			{
				basePos = endPos;
			}
		}

		pkVec_strToken->emplace_back(line.substr(beginPos, endPos - beginPos));
	} while (basePos < line.length());

	return !pkVec_strToken->empty();
}

bool SplitLine(std::string_view line, std::string_view delimiter, std::vector<std::string> & vec_strToken)
{
	return SplitLine(line, delimiter, &vec_strToken);
}

std::vector<std::string> SplitLine(std::string_view line, std::string_view delimiter)
{
	std::vector<std::string> tokens;
	SplitLine(line, delimiter, &tokens);
	return tokens;
}

bool SplitLine(const char * c_szLine, const char * c_szDelimeter, std::vector<std::string> * pkVec_strToken)
{
	if (!c_szLine || !c_szDelimeter || !pkVec_strToken)
		return false;
	return SplitLine(std::string_view(c_szLine), std::string_view(c_szDelimeter), pkVec_strToken);
}

// ============================================================================
// Executable Path
// ============================================================================

void GetExcutedFileName(std::string& r_str)
{
	wchar_t wPath[MAX_PATH + 1]{};
	GetModuleFileNameW(nullptr, wPath, MAX_PATH);
	wPath[MAX_PATH] = L'\0';
	r_str = WideToUtf8(wPath);
}

void GetExecutedFileName(std::string& r_str)
{
	GetExcutedFileName(r_str);
}

std::string GetExecutedFileName()
{
	std::string name;
	GetExcutedFileName(name);
	return name;
}

std::filesystem::path GetExecutedFilePath()
{
	wchar_t wPath[MAX_PATH + 1]{};
	GetModuleFileNameW(nullptr, wPath, MAX_PATH);
	wPath[MAX_PATH] = L'\0';
	return std::filesystem::path(wPath);
}

// ============================================================================
// Formatting
// ============================================================================

const char * _getf(const char* c_szFormat, ...)
{
	thread_local char szBuf[4096];

	va_list args;
	va_start(args, c_szFormat);
	vsnprintf(szBuf, sizeof(szBuf), c_szFormat, args);
	va_end(args);
	szBuf[sizeof(szBuf) - 1] = '\0';

	return szBuf;
}

std::string formatf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	va_list argsCopy;
	va_copy(argsCopy, args);

	int len = vsnprintf(nullptr, 0, format, args);
	va_end(args);

	if (len <= 0)
	{
		va_end(argsCopy);
		return {};
	}

	std::string result(static_cast<size_t>(len), '\0');
	vsnprintf(result.data(), len + 1, format, argsCopy);
	va_end(argsCopy);
	return result;
}

// ============================================================================
// Command Line Parsing
// ============================================================================

PCHAR* CommandLineToArgv( PCHAR CmdLine, int* _argc )
{
	PCHAR* argv;
	PCHAR  _argv;
	ULONG   len;
	ULONG   argc;
	CHAR   a;
	ULONG   i, j;

	BOOLEAN  in_QM;
	BOOLEAN  in_TEXT;
	BOOLEAN  in_SPACE;

	len = strlen(CmdLine);
	i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

	argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
		i + (len+2)*sizeof(CHAR));

	_argv = (PCHAR)(((PUCHAR)argv)+i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = FALSE;
	in_TEXT = FALSE;
	in_SPACE = TRUE;
	i = 0;
	j = 0;

	while( a = CmdLine[i] ) {
		if(in_QM) {
			if(a == '\"') {
				in_QM = FALSE;
			} else {
				_argv[j] = a;
				j++;
			}
		} else {
			switch(a) {
				case '\"':
					in_QM = TRUE;
					in_TEXT = TRUE;
					if(in_SPACE) {
						argv[argc] = _argv+j;
						argc++;
					}
					in_SPACE = FALSE;
					break;
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					if(in_TEXT) {
						_argv[j] = '\0';
						j++;
					}
					in_TEXT = FALSE;
					in_SPACE = TRUE;
					break;
				default:
					in_TEXT = TRUE;
					if(in_SPACE) {
						argv[argc] = _argv+j;
						argc++;
					}
					_argv[j] = a;
					j++;
					in_SPACE = FALSE;
					break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
}

std::vector<std::string> CommandLineToArgv(std::string_view cmdLine)
{
	std::vector<std::string> args;
	std::string current;
	bool inQuotes = false;

	for (size_t i = 0; i < cmdLine.size(); ++i)
	{
		char c = cmdLine[i];
		if (c == '"')
		{
			inQuotes = !inQuotes;
		}
		else if (!inQuotes && (c == ' ' || c == '\t' || c == '\r' || c == '\n'))
		{
			if (!current.empty())
			{
				args.push_back(std::move(current));
				current.clear();
			}
		}
		else
		{
			current += c;
		}
	}

	if (!current.empty())
		args.push_back(std::move(current));

	return args;
}

#ifndef __INC_ETER2_ETERBASE_UTILS_H__
#define __INC_ETER2_ETERBASE_UTILS_H__

#include <windows.h>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <memory>
#include <type_traits>
#include <optional>
#include <utility>
#include <span>
#include <cassert>
#include <cmath>

#include "Stl.h"

// Forward declaration for logging/assert macro
extern void TraceError(const char * c_szFormat, ...);

// ============================================================================
// Modern C++ RAII Deallocation & Release Templates
// ============================================================================

template <typename T>
inline void safe_delete(T*& p) noexcept
{
	static_assert(!std::is_void_v<T>, "safe_delete: cannot delete pointer to void");
	static_assert(sizeof(T) > 0, "safe_delete: cannot delete pointer to incomplete type");
	if (p)
	{
		delete p;
		p = nullptr;
	}
}

template <typename T>
inline void safe_delete_array(T*& p) noexcept
{
	static_assert(!std::is_void_v<T>, "safe_delete_array: cannot delete pointer to void");
	static_assert(sizeof(T) > 0, "safe_delete_array: cannot delete pointer to incomplete type");
	if (p)
	{
		delete[] p;
		p = nullptr;
	}
}

template <typename T>
inline void safe_free_global(T& p) noexcept
{
	if (p)
	{
		::GlobalFree(static_cast<HGLOBAL>(p));
		p = nullptr;
	}
}

template <typename T>
inline void safe_free_library(T& p) noexcept
{
	if (p)
	{
		::FreeLibrary(p);
		p = nullptr;
	}
}

// RAII Deleters for use with std::unique_ptr
template <typename T>
struct SafeDeleter
{
	constexpr void operator()(T* p) const noexcept
	{
		if (p)
			delete p;
	}
};

template <typename T>
struct SafeArrayDeleter
{
	constexpr void operator()(T* p) const noexcept
	{
		if (p)
			delete[] p;
	}
};

struct SafeReleaseDeleter
{
	template <typename T>
	void operator()(T* p) const noexcept
	{
		if (p)
			p->Release();
	}
};

struct SafeGlobalDeleter
{
	void operator()(void* p) const noexcept
	{
		if (p)
			::GlobalFree(static_cast<HGLOBAL>(p));
	}
};

struct SafeLibraryDeleter
{
	void operator()(HMODULE h) const noexcept
	{
		if (h)
			::FreeLibrary(h);
	}
};

template <typename T>
using unique_release_ptr = std::unique_ptr<T, SafeReleaseDeleter>;

// Backwards compatibility macros safely routed to inline templates
#ifdef SAFE_DELETE
#undef SAFE_DELETE
#endif
#define SAFE_DELETE(p) ::safe_delete(p)

#ifdef SAFE_DELETE_ARRAY
#undef SAFE_DELETE_ARRAY
#endif
#define SAFE_DELETE_ARRAY(p) ::safe_delete_array(p)

#ifdef SAFE_RELEASE
#undef SAFE_RELEASE
#endif
#define SAFE_RELEASE(p) ::safe_release(p)

#ifdef SAFE_FREE_GLOBAL
#undef SAFE_FREE_GLOBAL
#endif
#define SAFE_FREE_GLOBAL(p) ::safe_free_global(p)

#ifdef SAFE_FREE_LIBRARY
#undef SAFE_FREE_LIBRARY
#endif
#define SAFE_FREE_LIBRARY(p) ::safe_free_library(p)

#define AssertLog(str) do { TraceError(str); assert(!str); } while (false)

// ============================================================================
// Bit manipulation helpers
// ============================================================================

template <typename T, typename U>
constexpr bool is_set(T flag, U bit) noexcept
{
	return (flag & static_cast<T>(bit)) != 0;
}

template <typename T, typename U>
constexpr void set_bit(T& var, U bit) noexcept
{
	var |= static_cast<T>(bit);
}

template <typename T, typename U>
constexpr void remove_bit(T& var, U bit) noexcept
{
	var &= ~static_cast<T>(bit);
}

template <typename T, typename U>
constexpr void toggle_bit(T& var, U bit) noexcept
{
	var ^= static_cast<T>(bit);
}

#ifndef IS_SET
#define IS_SET(flag, bit) ((flag) & (bit))
#endif

#ifndef SET_BIT
#define SET_BIT(var, bit) ((var) |= (bit))
#endif

#ifndef REMOVE_BIT
#define REMOVE_BIT(var, bit) ((var) &= ~(bit))
#endif

#ifndef TOGGLE_BIT
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))
#endif

// ============================================================================
// File Path & String Utilities
// ============================================================================

struct FileNameParts
{
	std::string path;
	std::string name;
	std::string ext;
};

// Temp file creation
extern const char * CreateTempFileName(const char * c_pszPrefix = NULL);
std::string CreateTempFileName(std::string_view prefix);
std::filesystem::path CreateTempFilePath(std::string_view prefix = "etb");

// Path & extension splitting
extern void GetFilePathNameExtension(const char* c_szFile, int len, std::string* pstPath, std::string* pstName, std::string* pstExt);
void GetFilePathNameExtension(std::string_view file, std::string* pstPath, std::string* pstName, std::string* pstExt);
void GetFilePathNameExtension(std::string_view file, std::string& rstPath, std::string& rstName, std::string& rstExt);
FileNameParts GetFilePathNameExtension(std::string_view file);
void GetFilePathNameExtension(const std::filesystem::path& path, std::string* pstPath, std::string* pstName, std::string* pstExt);
void GetFilePathNameExtension(const std::filesystem::path& path, std::string& rstPath, std::string& rstName, std::string& rstExt);
FileNameParts GetFilePathNameExtension(const std::filesystem::path& path);

// File extension extraction
extern void GetFileExtension(const char* c_szFile, int len, std::string* pstExt);
void GetFileExtension(std::string_view file, std::string* pstExt);
void GetFileExtension(std::string_view file, std::string& rstExt);
std::string GetFileExtension(std::string_view file);
void GetFileExtension(const std::filesystem::path& path, std::string* pstExt);
void GetFileExtension(const std::filesystem::path& path, std::string& rstExt);
std::string GetFileExtension(const std::filesystem::path& path);

// File name parts (raw buffer & modern overloads)
extern void GetFileNameParts(const char* c_szFile, int len, char* pszPath, char* pszName, char* pszExt);
void GetFileNameParts(const char* c_szFile, size_t fileLen, char* pszPath, size_t pathLen, char* pszName, size_t nameLen, char* pszExt, size_t extLen);
void GetFileNameParts(std::string_view file, std::string& rPath, std::string& rName, std::string& rExt);
FileNameParts GetFileNameParts(std::string_view file);
void GetFileNameParts(const std::filesystem::path& path, std::string& rPath, std::string& rName, std::string& rExt);
FileNameParts GetFileNameParts(const std::filesystem::path& path);

// Indexing names
extern void GetOldIndexingName(char * szName, int Index);
void GetOldIndexingName(char * szName, size_t maxLen, int Index);
void GetOldIndexingName(std::string& rName, int Index);
std::string GetOldIndexingName(int Index);

extern void GetIndexingName(char * szName, DWORD Index);
void GetIndexingName(char * szName, size_t maxLen, DWORD Index);
void GetIndexingName(std::string& rName, DWORD Index);
std::string GetIndexingName(DWORD Index);

// Lowercase conversions
extern void stl_lowers(std::string& rstRet);
std::string stl_lowers(std::string_view str);

// Only filename extraction
extern void GetOnlyFileName(const char * sz_Name, std::string & strFileName);
void GetOnlyFileName(std::string_view name, std::string & strFileName);
void GetOnlyFileName(const std::filesystem::path& path, std::string & strFileName);
std::string GetOnlyFileName(std::string_view name);
std::string GetOnlyFileName(const std::filesystem::path& path);

// Only pathname extraction
extern void GetOnlyPathName(const char * sz_Name, std::string & OnlyPathName);
extern const char * GetOnlyPathName(const char * c_szName);
void GetOnlyPathName(std::string_view name, std::string & OnlyPathName);
void GetOnlyPathName(const std::filesystem::path& path, std::string & OnlyPathName);
std::string GetOnlyPathName(std::string_view name);
std::string GetOnlyPathName(const std::filesystem::path& path);

// Local file name relative to global path
bool GetLocalFileName(const char * c_szGlobalPath, const char * c_szFullPathFileName, std::string * pstrLocalFileName);
bool GetLocalFileName(std::string_view globalPath, std::string_view fullPathFileName, std::string* pstrLocalFileName);
bool GetLocalFileName(std::string_view globalPath, std::string_view fullPathFileName, std::string& rstrLocalFileName);
bool GetLocalFileName(const std::filesystem::path& globalPath, const std::filesystem::path& fullPathFileName, std::filesystem::path& rLocalFileName);
std::optional<std::string> GetLocalFileName(std::string_view globalPath, std::string_view fullPathFileName);
std::optional<std::filesystem::path> GetLocalFileName(const std::filesystem::path& globalPath, const std::filesystem::path& fullPathFileName);

// Exception path name
extern void GetExceptionPathName(const char * sz_Name, std::string & OnlyFileName);
void GetExceptionPathName(std::string_view name, std::string & OnlyFileName);
void GetExceptionPathName(const std::filesystem::path& path, std::string & OnlyFileName);
std::string GetExceptionPathName(std::string_view name);
std::string GetExceptionPathName(const std::filesystem::path& path);

// Working directory
extern void GetWorkingFolder(std::string & strFileName);
std::string GetWorkingFolder();
std::filesystem::path GetWorkingFolderPath();

// String lowers & path normalization
extern void StringLowers(char * pString);
void StringLowers(char * pString, size_t maxLen);
void StringLowers(std::string & rString);
std::string StringLowers(std::string_view str);

extern void StringPath(std::string & rString);
extern void StringPath(char * pString);
extern void StringPath(const char * c_szSrc, char * szDest);
extern void StringPath(const char * c_szSrc, std::string & rString);
void StringPath(const char * c_szSrc, char * szDest, size_t destLen);
void StringPath(std::string_view src, std::string & rString);
std::string StringPath(std::string_view src);
std::filesystem::path StringPath(const std::filesystem::path& path);

// Data dump
extern void PrintAsciiData(const void* data, int bytes);
void PrintAsciiData(std::span<const uint8_t> data);
void PrintAsciiData(std::string_view data);

// File checks
bool IsFile(const char* filename);
bool IsFile(std::string_view filename);
bool IsFile(const std::filesystem::path& path);

bool IsGlobalFileName(const char * c_szFileName);
bool IsGlobalFileName(std::string_view filename);
bool IsGlobalFileName(const std::filesystem::path& path);

// Math helpers
int MIN(int a, int b);
int MAX(int a, int b);
int MINMAX(int min, int value, int max);
float fMIN(float a, float b);
float fMAX(float a, float b);
float fMINMAX(float min, float value, float max);

template <typename T>
constexpr const T& (tMIN)(const T& a, const T& b) noexcept
{
	return (a < b) ? a : b;
}

template <typename T>
constexpr const T& (tMAX)(const T& a, const T& b) noexcept
{
	return (a > b) ? a : b;
}

template <typename T>
constexpr T tMINMAX(T minVal, T value, T maxVal) noexcept
{
	if (maxVal < minVal)
		return (value > minVal) ? value : minVal;
	return (value < minVal) ? minVal : ((value > maxVal) ? maxVal : value);
}

// Directory creation / removal
void MyCreateDirectory(const char* path);
bool MyCreateDirectory(std::string_view path);
bool MyCreateDirectory(const std::filesystem::path& path);

void RemoveAllDirectory(const char * c_szDirectoryName);
void RemoveAllDirectory(std::string_view directoryName);
void RemoveAllDirectory(const std::filesystem::path& path);

// String splitting
bool SplitLine(const char * c_szLine, const char * c_szDelimeter, std::vector<std::string> * pkVec_strToken);
bool SplitLine(std::string_view line, std::string_view delimiter, std::vector<std::string> * pkVec_strToken);
bool SplitLine(std::string_view line, std::string_view delimiter, std::vector<std::string> & vec_strToken);
std::vector<std::string> SplitLine(std::string_view line, std::string_view delimiter = " \t");

// Formatted string
const char * _getf(const char* c_szFormat, ...);
std::string formatf(const char* format, ...);

// Command line parsing
PCHAR* CommandLineToArgv( PCHAR CmdLine, int* _argc );
std::vector<std::string> CommandLineToArgv(std::string_view cmdLine);

// Angle & coordinate math
template<typename T>
T EL_DegreeToRadian(T degree)
{
	constexpr T PI = static_cast<T>(3.14159265358979323846);
	return static_cast<T>(PI * degree / 180.0f);
}

template<typename T>
void ELPlainCoord_GetRotatedPixelPosition(T centerX, T centerY, T distance, T rotDegree, T* pdstX, T* pdstY)
{
	T rotRadian = EL_DegreeToRadian(rotDegree);
	*pdstX = centerX + distance * static_cast<T>(sin(static_cast<double>(rotRadian)));
	*pdstY = centerY + distance * static_cast<T>(cos(static_cast<double>(rotRadian)));
}

template<typename T>
T EL_SignedDegreeToUnsignedDegree(T fSrc)
{
	if (fSrc < 0.0f)
		return static_cast<T>(360.0 + static_cast<T>(fmod(fSrc, 360.0)));

	return static_cast<T>(fmod(fSrc, 360.0));
}

template<typename T>
T ELRightCoord_ConvertToPlainCoordDegree(T srcDegree)
{
	return static_cast<T>(fmod(450.0 - srcDegree, 360.0));
}

template<typename C>
void string_join(const std::string& sep, const C& container, std::string* ret)
{
	if (!ret)
		return;

	if (container.empty())
	{
		ret->clear();
		return;
	}

	size_t capacity = sep.length() * (container.size() - 1);
	for (auto i = container.begin(); i != container.end(); ++i)
		capacity += (*i).length();

	std::string buf;
	buf.reserve(capacity);

	auto cur = container.begin();
	auto end = container.end();
	--end;

	while (cur != end)
	{
		buf.append(*cur++);
		buf.append(sep);
	}
	buf.append(*cur);

	*ret = std::move(buf);
}

template<typename C>
inline std::string string_join(std::string_view sep, const C& container)
{
	std::string ret;
	if (container.empty())
		return ret;

	size_t capacity = sep.length() * (container.size() - 1);
	for (const auto& item : container)
		capacity += item.length();

	ret.reserve(capacity);
	auto it = container.begin();
	ret.append(*it++);
	for (; it != container.end(); ++it)
	{
		ret.append(sep);
		ret.append(*it);
	}
	return ret;
}

__forceinline int htoi(const wchar_t *s, int size)
{
	const wchar_t *t = s;
	int x = 0, y = 1;
	s += size;

	while (t <= --s)
	{
		if (L'0' <= *s && *s <= L'9')
			x += y * (*s - L'0');
		else if (L'a' <= *s && *s <= L'f')
			x += y * (*s - L'a' + 10);
		else if (L'A' <= *s && *s <= L'F')
			x += y * (10 + *s - L'A');
		else
			return -1;
		y <<= 4;
	}

	return x;
}

__forceinline int htoi(const char *s, int size)
{
	const char *t = s;
	int x = 0, y = 1;
	s += size;

	while (t <= --s)
	{
		if ('0' <= *s && *s <= '9')
			x += y * (*s - '0');
		else if ('a' <= *s && *s <= 'f')
			x += y * (*s - 'a' + 10);
		else if ('A' <= *s && *s <= 'F')
			x += y * (10 + *s - 'A');
		else
			return -1;
		y <<= 4;
	}

	return x;
}

__forceinline int htoi(const char *s)
{
	if (!s)
		return -1;
	return htoi(s, static_cast<int>(strlen(s)));
}

__forceinline int htoi(std::string_view s)
{
	return htoi(s.data(), static_cast<int>(s.size()));
}

__forceinline int htoi(std::wstring_view s)
{
	return htoi(s.data(), static_cast<int>(s.size()));
}

typedef std::vector<std::string> TTokenVector;

void StringExceptCharacter(std::string * pstrString, const char * c_szCharacter);
void StringExceptCharacter(std::string & rString, std::string_view characters);
std::string StringExceptCharacter(std::string_view str, std::string_view characters);

extern void GetExcutedFileName(std::string & r_str);
void GetExecutedFileName(std::string & r_str);
std::string GetExecutedFileName();
std::filesystem::path GetExecutedFilePath();

template<typename T>
constexpr T LinearInterpolation(const T& tMin, const T& tMax, float fRatio)
{
	return static_cast<T>(tMin * (1.0f - fRatio) + tMax * fRatio);
}

template<typename T>
constexpr T HermiteInterpolation(const T& tMin, const T& tMax, float fRatio)
{
	fRatio = fMINMAX(0.0f, fRatio, 1.0f);
	fRatio = fRatio * fRatio * (3.0f - 2.0f * fRatio);
	return LinearInterpolation(tMin, tMax, fRatio);
}

#endif

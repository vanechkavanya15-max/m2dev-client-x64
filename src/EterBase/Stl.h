#ifndef __INC_ETERBASE_STL_H__
#define __INC_ETERBASE_STL_H__

#include <cassert>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <stack>
#include <deque>
#include <list>
#include <set>
#include <map>
#include <queue>
#include <functional>
#include <sstream>
#include <ranges>
#include <concepts>
#include <type_traits>
#include <utility>
#include <cctype>
#include <cstring>

// Podstawowe funkcje pomocnicze
extern char ascii_tolower(const char c);
extern std::string& stl_static_string(const char* c_sz);
extern void stl_lowers(std::string& rstRet);

// split_string - pelna kompatybilnosc wsteczna
extern int split_string(const std::string& input, const std::string& delimiter, std::vector<std::string>& results, bool includeEmpties);

// split_string - szybka bezalokacyjna wersja z std::string_view
extern int split_string(std::string_view input, std::string_view delimiter, std::vector<std::string_view>& results, bool includeEmpties = false);

// Nowoczesny podzial oparty bezposrednio na std::ranges::views::split (C++20/C++23)
[[nodiscard]] inline auto split_string_views(std::string_view input, std::string_view delimiter)
{
	return input | std::views::split(delimiter);
}

// Szybka, bezalokacyjna wersja oparta o callback (zero alokacji pamieci na stercie)
template <typename Callback>
	requires std::invocable<Callback, std::string_view>
inline int split_string_view_each(std::string_view input, std::string_view delimiter, Callback&& callback, bool includeEmpties = false)
{
	if (input.empty() || delimiter.empty())
		return 0;

	const size_t firstPos = input.find(delimiter);
	if (firstPos == std::string_view::npos)
		return 0;

	int numFound = 0;
	size_t curPos = 0;

	while (curPos <= input.size())
	{
		const size_t nextPos = input.find(delimiter, curPos);
		if (nextPos == std::string_view::npos)
		{
			std::string_view token = input.substr(curPos);
			if (includeEmpties || !token.empty())
				callback(token);
			break;
		}

		++numFound;
		std::string_view token = input.substr(curPos, nextPos - curPos);
		if (includeEmpties || !token.empty())
			callback(token);

		curPos = nextPos + delimiter.size();
		if (curPos == input.size())
		{
			if (includeEmpties)
				callback(std::string_view{});
			break;
		}
	}

	return numFound;
}

// Iteracja po tokenach bezposrednio z wykorzystaniem std::ranges::views::split
template <typename Callback>
	requires std::invocable<Callback, std::string_view>
inline void split_range_for_each(std::string_view input, std::string_view delimiter, Callback&& callback, bool includeEmpties = false)
{
	for (auto&& part : input | std::views::split(delimiter))
	{
		std::string_view token{part.begin(), part.end()};
		if (includeEmpties || !token.empty())
		{
			callback(token);
		}
	}
}

struct stl_sz_less
{
	using is_transparent = void;

	bool operator()(const char* left, const char* right) const noexcept
	{
		return (strcmp(left, right) < 0);
	}

	bool operator()(char* const& left, char* const& right) const noexcept
	{
		return (strcmp(left, right) < 0);
	}

	bool operator()(std::string_view left, std::string_view right) const noexcept
	{
		return left < right;
	}
};

// Koncepcje dla kontenerow i zarzadzania wskaznikami
template <typename T>
concept ClearableContainer = std::ranges::range<T> && requires(T& c) {
	c.clear();
};

// Modernizacja stl_wipe z uzyciem std::ranges i concepts
// Gwarantuje zwolnienie pamieci i zerowanie wskaznikow
template <ClearableContainer TContainer>
inline void stl_wipe(TContainer& container)
{
	std::ranges::for_each(container, [](auto& item) {
		if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(item)>>)
		{
			if (item)
			{
				delete item;
				if constexpr (std::is_assignable_v<decltype(item)&, std::nullptr_t>)
				{
					item = nullptr;
				}
			}
		}
		else
		{
			delete item;
		}
	});

	container.clear();
}

template <ClearableContainer TContainer>
inline void stl_wipe(TContainer* pContainer)
{
	if (pContainer)
	{
		stl_wipe(*pContainer);
	}
}

template <typename TString>
[[nodiscard]] constexpr int hex2dec(const TString& szhex) noexcept
{
	const int hex0 = (szhex[0] >= 'a' && szhex[0] <= 'f') ? (szhex[0] - 'a' + 10) :
	                 (szhex[0] >= 'A' && szhex[0] <= 'F') ? (szhex[0] - 'A' + 10) : (szhex[0] - '0');
	const int hex1 = (szhex[1] >= 'a' && szhex[1] <= 'f') ? (szhex[1] - 'a' + 10) :
	                 (szhex[1] >= 'A' && szhex[1] <= 'F') ? (szhex[1] - 'A' + 10) : (szhex[1] - '0');

	return hex0 * 16 + hex1;
}

template <typename TString>
[[nodiscard]] constexpr unsigned long htmlColorStringToARGB(const TString& str) noexcept
{
	const unsigned long alp   = hex2dec(str);
	const unsigned long red   = hex2dec(str + 2);
	const unsigned long green = hex2dec(str + 4);
	const unsigned long blue  = hex2dec(str + 6);
	return (alp << 24 | red << 16 | green << 8 | blue);
}

// Modernizacja stl_wipe_second z uzyciem std::ranges i concepts dla map i asocjacji
template <ClearableContainer TContainer>
inline void stl_wipe_second(TContainer& container)
{
	std::ranges::for_each(container, [](auto& pair) {
		auto& val = pair.second;
		if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(val)>>)
		{
			if (val)
			{
				delete val;
				if constexpr (std::is_assignable_v<decltype(val)&, std::nullptr_t>)
				{
					val = nullptr;
				}
			}
		}
		else
		{
			delete val;
		}
	});

	container.clear();
}

template <ClearableContainer TContainer>
inline void stl_wipe_second(TContainer* pContainer)
{
	if (pContainer)
	{
		stl_wipe_second(*pContainer);
	}
}

template <typename T>
inline void safe_release(T& rpObject) noexcept
{
	if (rpObject)
	{
		rpObject->Release();
		rpObject = nullptr;
	}
}

template <typename T>
inline void DeleteVectorItem(std::vector<T>* pVector, unsigned long dwIndex)
{
	if (!pVector)
		return;

	if (dwIndex >= pVector->size())
	{
		assert(!"Wrong index to delete!");
		return;
	}

	pVector->erase(pVector->begin() + dwIndex);
}

template <typename T>
inline void DeleteVectorItem(std::vector<T>& rVector, unsigned long dwIndex)
{
	if (dwIndex >= rVector.size())
	{
		assert(!"Wrong index to delete!");
		return;
	}

	rVector.erase(rVector.begin() + dwIndex);
}

template <typename T>
inline void DeleteVectorItem(T* pVector, unsigned long dwStartIndex, unsigned long dwEndIndex)
{
	if (!pVector)
		return;

	if (dwStartIndex >= pVector->size())
	{
		assert(!"Wrong start index to delete!");
		return;
	}
	if (dwEndIndex > pVector->size() || dwStartIndex > dwEndIndex)
	{
		assert(!"Wrong end index to delete!");
		return;
	}

	auto itorStart = pVector->begin();
	std::advance(itorStart, dwStartIndex);
	auto itorEnd = pVector->begin();
	std::advance(itorEnd, dwEndIndex);

	pVector->erase(itorStart, itorEnd);
}

template <typename T>
inline void DeleteVectorItem(std::vector<T>* pVector, const T& pItem)
{
	if (!pVector)
		return;

	auto it = std::ranges::find(*pVector, pItem);
	if (it != pVector->end())
	{
		pVector->erase(it);
	}
}

template <typename T>
inline void DeleteVectorItem(std::vector<T>& rVector, const T& pItem)
{
	auto it = std::ranges::find(rVector, pItem);
	if (it != rVector.end())
	{
		rVector.erase(it);
	}
}

template <typename T>
inline void DeleteListItem(std::list<T>* pList, const T& pItem)
{
	if (!pList)
		return;

	auto it = std::ranges::find(*pList, pItem);
	if (it != pList->end())
	{
		pList->erase(it);
	}
}

template <typename T>
inline void DeleteListItem(std::list<T>& rList, const T& pItem)
{
	auto it = std::ranges::find(rList, pItem);
	if (it != rList.end())
	{
		rList.erase(it);
	}
}

template <typename T, typename F>
inline void stl_vector_qsort(std::vector<T>& rdataVector, F comp)
{
	if (rdataVector.empty())
		return;
	qsort(rdataVector.data(), rdataVector.size(), sizeof(T), comp);
}

template <typename T, typename Comp = std::less<T>>
inline void stl_vector_sort(std::vector<T>& rdataVector, Comp comp = Comp{})
{
	std::ranges::sort(rdataVector, comp);
}

template <typename TData>
class stl_stack_pool
{
public:
	stl_stack_pool() : m_pos(0) {}

	explicit stl_stack_pool(int capacity) : m_pos(0)
	{
		initialize(capacity);
	}

	virtual ~stl_stack_pool() = default;

	void initialize(int capacity)
	{
		m_dataVector.clear();
		m_dataVector.resize(capacity);
		m_pos = 0;
	}

	void clear() noexcept
	{
		m_pos = 0;
	}

	[[nodiscard]] TData* alloc()
	{
		assert(!m_dataVector.empty() && "stl_stack_pool::alloc you MUST run stl_stack_pool::initialize");

		const int max = static_cast<int>(m_dataVector.size());
		if (m_pos >= max)
		{
			assert(!"stl_stack_pool::alloc OUT of memory");
			m_pos = 0;
		}

		return &m_dataVector[m_pos++];
	}

	[[nodiscard]] TData* base() noexcept
	{
		return m_dataVector.data();
	}

	[[nodiscard]] const TData* base() const noexcept
	{
		return m_dataVector.data();
	}

	[[nodiscard]] int size() const noexcept
	{
		return m_pos;
	}

	[[nodiscard]] size_t capacity() const noexcept
	{
		return m_dataVector.size();
	}

private:
	int m_pos{0};
	std::vector<TData> m_dataVector;
};

template <typename TData, typename THandle = int>
class stl_circle_pool
{
public:
	using TFlag = bool;

	stl_circle_pool()
	{
		initialize();
	}

	virtual ~stl_circle_pool()
	{
		destroy();
	}

	void destroy()
	{
		if (m_datas)
		{
			delete[] m_datas;
			m_datas = nullptr;
		}
		if (m_flags)
		{
			delete[] m_flags;
			m_flags = nullptr;
		}
		m_size = 0;
		m_pos = 0;
	}

	void create(int size)
	{
		destroy();

		initialize();

		m_size = static_cast<THandle>(size);
		if (m_size > 0)
		{
			m_datas = new TData[m_size]();
			m_flags = new TFlag[m_size]();
		}
	}

	THandle alloc()
	{
		const THandle max = m_size;
		THandle loop = max;
		while (loop--)
		{
			const int cur = static_cast<int>(m_pos % max);
			++m_pos;
			if (!m_flags[cur])
			{
				m_flags[cur] = true;
				return static_cast<THandle>(cur);
			}
		}

		assert(!"Out of Memory");
		return 0;
	}

	void free(THandle handle)
	{
		assert(check(handle) && "Out of RANGE");
		if (check(handle))
		{
			m_flags[handle] = false;
		}
	}

	[[nodiscard]] inline bool check(THandle handle) const noexcept
	{
		return handle >= 0 && handle < m_size;
	}

	[[nodiscard]] inline int size() const noexcept
	{
		return static_cast<int>(m_size);
	}

	[[nodiscard]] inline TData& refer(THandle handle)
	{
		assert(check(handle) && "Out of RANGE");
		return m_datas[handle];
	}

	[[nodiscard]] inline const TData& refer(THandle handle) const
	{
		assert(check(handle) && "Out of RANGE");
		return m_datas[handle];
	}

protected:
	void initialize() noexcept
	{
		m_datas = nullptr;
		m_flags = nullptr;
		m_pos = 0;
		m_size = 0;
	}

protected:
	TData*  m_datas{nullptr};
	TFlag*  m_flags{nullptr};
	THandle m_size{0};
	THandle m_pos{0};
};

using CTokenVector = std::vector<std::string>;
using CTokenMap = std::map<std::string, std::string>;
using CTokenVectorMap = std::map<std::string, CTokenVector>;
using CTokenVectorView = std::vector<std::string_view>;

struct stringhash
{
	[[nodiscard]] constexpr size_t GetHash(std::string_view str) const noexcept
	{
		size_t h = 0;
		for (const unsigned char c : str)
		{
			h *= 16777619u;
			h ^= static_cast<size_t>(c);
		}
		return h;
	}

	[[nodiscard]] constexpr size_t operator()(std::string_view str) const noexcept
	{
		return GetHash(str);
	}
};

#endif

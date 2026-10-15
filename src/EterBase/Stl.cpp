#include "StdAfx.h"
#include "Stl.h"

static std::list<std::string> s_stList;

char ascii_tolower(const char c)
{
	if (c >= 'A' && c <= 'Z')
		return static_cast<char>(c - 'A' + 'a');
	return c;
}

std::string& stl_static_string(const char* c_sz)
{
	s_stList.emplace_back(c_sz ? c_sz : "");
	return s_stList.back();
}

void stl_lowers(std::string& rstRet)
{
	for (char& c : rstRet)
		c = ascii_tolower(c);
}

int split_string(const std::string& input, const std::string& delimiter, std::vector<std::string>& results, bool includeEmpties)
{
	const std::string_view svInput = input;
	const std::string_view svDelim = delimiter;

	if (svInput.empty() || svDelim.empty())
		return 0;

	const size_t firstPos = svInput.find(svDelim);
	if (firstPos == std::string_view::npos)
		return 0;

	int numFound = 0;
	size_t curPos = 0;

	while (curPos <= svInput.size())
	{
		const size_t nextPos = svInput.find(svDelim, curPos);
		if (nextPos == std::string_view::npos)
		{
			const std::string_view token = svInput.substr(curPos);
			if (includeEmpties || !token.empty())
				results.emplace_back(token);
			break;
		}

		++numFound;
		const std::string_view token = svInput.substr(curPos, nextPos - curPos);
		if (includeEmpties || !token.empty())
			results.emplace_back(token);

		curPos = nextPos + svDelim.size();
		if (curPos == svInput.size())
		{
			if (includeEmpties)
				results.emplace_back("");
			break;
		}
	}

	return numFound;
}

int split_string(std::string_view input, std::string_view delimiter, std::vector<std::string_view>& results, bool includeEmpties)
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
			const std::string_view token = input.substr(curPos);
			if (includeEmpties || !token.empty())
				results.push_back(token);
			break;
		}

		++numFound;
		const std::string_view token = input.substr(curPos, nextPos - curPos);
		if (includeEmpties || !token.empty())
			results.push_back(token);

		curPos = nextPos + delimiter.size();
		if (curPos == input.size())
		{
			if (includeEmpties)
				results.push_back({});
			break;
		}
	}

	return numFound;
}

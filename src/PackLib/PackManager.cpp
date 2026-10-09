#include "PackManager.h"
#include "EterLib/BufferPool.h"
#include <fstream>
#include <filesystem>
#include "EterBase/Debug.h"

CPackManager::CPackManager()
	: m_load_from_pack(true)
	, m_pBufferPool(nullptr)
{
	m_pBufferPool = new CBufferPool();
}

CPackManager::~CPackManager()
{
	if (m_pBufferPool)
	{
		delete m_pBufferPool;
		m_pBufferPool = nullptr;
	}
}

bool CPackManager::AddPack(const std::string& path)
{
	std::shared_ptr<CPack> pack = std::make_shared<CPack>();

	if (!pack->Load(path))
	{
		return false;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	const auto& index = pack->GetIndex();
	for (const auto& entry : index)
	{
		m_entries[entry.file_name] = std::make_pair(pack, entry);
	}

	return true;
}

bool CPackManager::GetFile(std::string_view path, TPackFile& result)
{
	return GetFileWithPool(path, result, m_pBufferPool);
}

bool CPackManager::GetFileWithPool(std::string_view path, TPackFile& result, CBufferPool* pPool)
{
	thread_local std::string buf;
	NormalizePath(path, buf);

	auto tryLoad = [&](const std::string& targetPath) -> bool {
		// First try to load from pack entries
		if (m_load_from_pack) {
			auto it = m_entries.find(targetPath);
			if (it != m_entries.end()) {
				return it->second.first->GetFileWithPool(it->second.second, result, pPool);
			}
		}

		// Fallback to disk (for loose files or dev mode)
		std::error_code ec;
		std::filesystem::path fspath = std::filesystem::u8path(targetPath);
		if (std::filesystem::exists(fspath, ec)) {
			std::ifstream ifs(fspath, std::ios::binary);
			if (ifs.is_open()) {
				ifs.seekg(0, std::ios::end);
				size_t size = ifs.tellg();
				ifs.seekg(0, std::ios::beg);

				if (pPool) {
					result = pPool->Acquire(size);
					result.resize(size);
				} else {
					result.resize(size);
				}

				if (ifs.read((char*)result.data(), size)) {
					return true;
				}
			}
		}
		return false;
	};

	// 1. Transparent migration: gdy proszony jest stary format .gr2, preferuj nowoczesny model .glb
	if (buf.size() > 4 && buf.ends_with(".gr2")) {
		std::string glbPath = buf.substr(0, buf.size() - 4) + ".glb";
		if (tryLoad(glbPath)) {
			return true;
		}
		// Fallback do oryginalnego .gr2 gdy .glb nie istnieje
		return tryLoad(buf);
	}

	// 2. Gdy proszony jest .glb, najpierw laduj .glb, z fallbackiem do .gr2
	if (buf.size() > 4 && buf.ends_with(".glb")) {
		if (tryLoad(buf)) {
			return true;
		}
		std::string gr2Path = buf.substr(0, buf.size() - 4) + ".gr2";
		return tryLoad(gr2Path);
	}

	// 3. Pozostale pliki (.dds, .tga, .txt, .py, .mse itp.)
	return tryLoad(buf);
}

bool CPackManager::IsExist(std::string_view path) const
{
	thread_local std::string buf;
	NormalizePath(path, buf);

	auto checkExist = [&](const std::string& targetPath) -> bool {
		if (m_load_from_pack) {
			if (m_entries.find(targetPath) != m_entries.end())
				return true;
		}
		std::error_code ec;
		return std::filesystem::exists(std::filesystem::u8path(targetPath), ec);
	};

	// 1. Transparent migration check dla .gr2
	if (buf.size() > 4 && buf.ends_with(".gr2")) {
		std::string glbPath = buf.substr(0, buf.size() - 4) + ".glb";
		if (checkExist(glbPath))
			return true;
		return checkExist(buf);
	}

	// 2. Transparent migration check dla .glb
	if (buf.size() > 4 && buf.ends_with(".glb")) {
		if (checkExist(buf))
			return true;
		std::string gr2Path = buf.substr(0, buf.size() - 4) + ".gr2";
		return checkExist(gr2Path);
	}

	return checkExist(buf);
}

void CPackManager::NormalizePath(std::string_view in, std::string& out) const
{
	out.resize(in.size());
	for (std::size_t i = 0; i < out.size(); ++i) {
		if (in[i] == '\\')
			out[i] = '/';
		else
			out[i] = static_cast<char>(std::tolower(in[i]));
	}
}

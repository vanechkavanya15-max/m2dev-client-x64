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

	// First try to load from pack
	if (m_load_from_pack) {
		auto it = m_entries.find(buf);
		if (it != m_entries.end()) {
			return it->second.first->GetFileWithPool(it->second.second, result, pPool);
		}
	}

	// Fallback to disk (for files not in packs, like bgm folder)
	std::ifstream ifs(buf, std::ios::binary);
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

	// Transparent Fallback dla migracji GR2 <-> GLB
	if (buf.size() > 4) {
		if (buf.ends_with(".gr2")) {
			std::string glbPath = buf.substr(0, buf.size() - 4) + ".glb";
			if (m_load_from_pack) {
				auto itGlb = m_entries.find(glbPath);
				if (itGlb != m_entries.end()) {
					return itGlb->second.first->GetFileWithPool(itGlb->second.second, result, pPool);
				}
			}
			std::ifstream ifsGlb(glbPath, std::ios::binary);
			if (ifsGlb.is_open()) {
				ifsGlb.seekg(0, std::ios::end);
				size_t size = ifsGlb.tellg();
				ifsGlb.seekg(0, std::ios::beg);
				if (pPool) {
					result = pPool->Acquire(size);
					result.resize(size);
				} else {
					result.resize(size);
				}
				if (ifsGlb.read((char*)result.data(), size)) {
					return true;
				}
			}
		} else if (buf.ends_with(".glb")) {
			std::string gr2Path = buf.substr(0, buf.size() - 4) + ".gr2";
			if (m_load_from_pack) {
				auto itGr2 = m_entries.find(gr2Path);
				if (itGr2 != m_entries.end()) {
					return itGr2->second.first->GetFileWithPool(itGr2->second.second, result, pPool);
				}
			}
			std::ifstream ifsGr2(gr2Path, std::ios::binary);
			if (ifsGr2.is_open()) {
				ifsGr2.seekg(0, std::ios::end);
				size_t size = ifsGr2.tellg();
				ifsGr2.seekg(0, std::ios::beg);
				if (pPool) {
					result = pPool->Acquire(size);
					result.resize(size);
				} else {
					result.resize(size);
				}
				if (ifsGr2.read((char*)result.data(), size)) {
					return true;
				}
			}
		}
	}

	return false;
}

bool CPackManager::IsExist(std::string_view path) const
{
	thread_local std::string buf;
	NormalizePath(path, buf);

	// First check in pack entries
	if (m_load_from_pack) {
		auto it = m_entries.find(buf);
		if (it != m_entries.end()) {
			return true;
		}
	}

	// Fallback to disk (for files not in packs, like bgm folder)
	std::error_code ec; // To avoid exceptions from std::filesystem
	if (std::filesystem::exists(buf, ec))
		return true;

	// Transparent fallback check for GR2 <-> GLB
	if (buf.size() > 4) {
		if (buf.ends_with(".gr2")) {
			std::string glbPath = buf.substr(0, buf.size() - 4) + ".glb";
			if (m_load_from_pack && m_entries.find(glbPath) != m_entries.end())
				return true;
			if (std::filesystem::exists(glbPath, ec))
				return true;
		} else if (buf.ends_with(".glb")) {
			std::string gr2Path = buf.substr(0, buf.size() - 4) + ".gr2";
			if (m_load_from_pack && m_entries.find(gr2Path) != m_entries.end())
				return true;
			if (std::filesystem::exists(gr2Path, ec))
				return true;
		}
	}

	return false;
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

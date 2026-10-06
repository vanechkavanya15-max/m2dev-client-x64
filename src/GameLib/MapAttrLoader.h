#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

/**
 * @brief Error codes for MapAttrLoader operations.
 */
enum class MapAttrLoadError : uint8_t
{
	Success = 0,
	FileOpenFailed,
	InvalidSize,
	InvalidMagicNumber,
	InvalidDimensions,
};

/**
 * @brief Represents the header of a Map Attribute (.atr) file.
 *
 * It uses exact 1-byte alignment.
 */
#pragma pack(push, 1)
struct MapAttrHeader
{
	uint16_t magic;  ///< Magic number indicating the start of an attribute map.
	uint16_t width;  ///< Width of the attribute map.
	uint16_t height; ///< Height of the attribute map.
};
#pragma pack(pop)

/**
 * @brief Holds the loaded header and data for a Map Attribute file.
 */
struct MapAttrData
{
	MapAttrHeader header;
	std::vector<uint8_t> attributes; ///< Flattened 2D array of attribute data.
};

/**
 * @brief A standalone reader for server_attr / attr map files, fully decoupled from the GUI.
 */
class MapAttrLoader
{
public:
	/**
	 * @brief Constant magic number for Map Attribute files.
	 */
	static constexpr uint16_t MAGIC_NUMBER = 2634;

	/**
	 * @brief Loads map attribute data from a raw binary buffer.
	 * 
	 * @param buffer A span of constant bytes containing the raw file data.
	 * @param outData The structured MapAttrData to populate on success.
	 * @return MapAttrLoadError::Success on successful load, or an error code on failure.
	 */
	static MapAttrLoadError LoadFromBuffer(std::span<const uint8_t> buffer, MapAttrData& outData);

	/**
	 * @brief Loads map attribute data directly from a file.
	 * 
	 * @param filePath The path to the attribute map file.
	 * @param outData The structured MapAttrData to populate on success.
	 * @return MapAttrLoadError::Success on successful load, or an error code on failure.
	 */
	static MapAttrLoadError LoadFromFile(std::string_view filePath, MapAttrData& outData);
};

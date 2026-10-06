#include "../../StdAfx.h"
/**
 * @file CubeCraftHandler.cpp
 * @brief Modern C++20 handler for Cube Crafting system network events.
 * 
 * This file implements the CubeCraftHandler class which encapsulates the
 * network state and logic for the cube crafting system, completely decoupled
 * from the Python GUI.
 */

#include <cstdint>
#include <string_view>
#include <vector>
#include <charconv>
#include <optional>
#include <algorithm>
#include <system_error>

namespace Network {
namespace Handlers {

/**
 * @brief Represents a single item required or produced in cube crafting.
 */
struct CubeItem {
    uint32_t vnum;   ///< Virtual number of the item
    uint32_t count;  ///< Quantity of the item
};

/**
 * @brief Represents the recipe for crafting a single result item.
 */
struct CubeRecipe {
    CubeItem result;                    ///< The item produced by this recipe
    std::vector<CubeItem> materials;    ///< The items required to craft the result
    uint32_t gold;                      ///< The cost in gold to craft
};

/**
 * @brief Manages the state and network logic for the Cube Crafting system.
 * 
 * This class decouples the UI from network handling by storing the crafting state
 * entirely in C++ memory. The UI can poll or observe this state to update itself.
 */
class CubeCraftHandler {
public:
    /**
     * @brief Constructor for CubeCraftHandler.
     */
    CubeCraftHandler() = default;

    /**
     * @brief Handles the opening of the cube crafting window.
     * @param npcVnum The virtual number of the NPC providing the crafting service.
     */
    void Open(uint32_t npcVnum);

    /**
     * @brief Handles the closing of the cube crafting window.
     */
    void Close();

    /**
     * @brief Updates the general info for the current crafting operation.
     * @param gold The gold required for crafting.
     * @param itemVnum The primary item vnum involved.
     * @param count The quantity of the item.
     */
    void UpdateInfo(uint32_t gold, uint32_t itemVnum, uint32_t count);

    /**
     * @brief Handles a successful crafting event.
     * @param itemVnum The vnum of the crafted item.
     * @param count The quantity of the crafted item.
     */
    void Succeed(uint32_t itemVnum, uint32_t count);

    /**
     * @brief Handles a failed crafting event.
     */
    void Failed();

    /**
     * @brief Parses the result list sent by the server.
     * 
     * @param npcVnum The NPC providing these recipes.
     * @param resultText The raw string view containing the result list.
     */
    void ParseResultList(uint32_t npcVnum, std::string_view resultText);

    /**
     * @brief Parses the material info list sent by the server.
     * 
     * @param startIndex The starting index in the recipe list this info applies to.
     * @param resultCount The number of recipes this info covers.
     * @param materialText The raw string view containing the materials and gold.
     */
    void ParseMaterialInfo(uint32_t startIndex, uint32_t resultCount, std::string_view materialText);

    /**
     * @brief Checks if the crafting window is currently open.
     * @return True if open, false otherwise.
     */
    [[nodiscard]] bool IsOpen() const;

    /**
     * @brief Retrieves the currently stored recipes.
     * @return A constant reference to the list of recipes.
     */
    [[nodiscard]] const std::vector<CubeRecipe>& GetRecipes() const;

private:
    std::optional<CubeItem> ParseCubeItem(std::string_view token, char separator) const;

    bool isOpen{false};
    uint32_t currentNpcVnum{0};
    uint32_t requiredGold{0};
    CubeItem activeItem{0, 0};
    CubeItem lastResult{0, 0};
    bool isLastResultSuccess{false};
    std::vector<CubeRecipe> recipes;
};

void CubeCraftHandler::Open(uint32_t npcVnum) {
    isOpen = true;
    currentNpcVnum = npcVnum;
    recipes.clear();
    requiredGold = 0;
}

void CubeCraftHandler::Close() {
    isOpen = false;
    currentNpcVnum = 0;
    recipes.clear();
    requiredGold = 0;
}

void CubeCraftHandler::UpdateInfo(uint32_t gold, uint32_t itemVnum, uint32_t count) {
    requiredGold = gold;
    activeItem.vnum = itemVnum;
    activeItem.count = count;
}

void CubeCraftHandler::Succeed(uint32_t itemVnum, uint32_t count) {
    lastResult = CubeItem{itemVnum, count};
    isLastResultSuccess = true;
}

void CubeCraftHandler::Failed() {
    isLastResultSuccess = false;
}

void CubeCraftHandler::ParseResultList(uint32_t npcVnum, std::string_view resultText) {
    if (npcVnum != currentNpcVnum) {
        return;
    }
    
    recipes.clear();

    size_t start = 0;
    while (start < resultText.length()) {
        size_t end = resultText.find('/', start);
        std::string_view token = (end == std::string_view::npos) ? 
            resultText.substr(start) : resultText.substr(start, end - start);

        if (auto itemOpt = ParseCubeItem(token, ','); itemOpt.has_value()) {
            CubeRecipe recipe;
            recipe.result = *itemOpt;
            recipe.gold = 0;
            recipes.push_back(std::move(recipe));
        }

        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
}

void CubeCraftHandler::ParseMaterialInfo(uint32_t startIndex, uint32_t resultCount, std::string_view materialText) {
    if (startIndex >= recipes.size()) {
        return;
    }

    size_t recipeStart = 0;
    uint32_t currentRecipeIdx = startIndex;
    uint32_t endIndex = startIndex + resultCount;

    while (recipeStart < materialText.length() && currentRecipeIdx < recipes.size() && currentRecipeIdx < endIndex) {
        size_t recipeEnd = materialText.find('@', recipeStart);
        std::string_view recipeText = (recipeEnd == std::string_view::npos) ? 
            materialText.substr(recipeStart) : materialText.substr(recipeStart, recipeEnd - recipeStart);

        size_t goldPos = recipeText.find('/');
        std::string_view matsText = recipeText;
        uint32_t gold = 0;

        if (goldPos != std::string_view::npos) {
            matsText = recipeText.substr(0, goldPos);
            std::string_view goldText = recipeText.substr(goldPos + 1);
            std::from_chars(goldText.data(), goldText.data() + goldText.size(), gold);
        }

        recipes[currentRecipeIdx].gold = gold;
        recipes[currentRecipeIdx].materials.clear();

        size_t matStart = 0;
        while (matStart < matsText.length()) {
            size_t matEnd1 = matsText.find('&', matStart);
            size_t matEnd2 = matsText.find('|', matStart);
            size_t matEnd = std::min(matEnd1, matEnd2);

            std::string_view matToken = (matEnd == std::string_view::npos) ? 
                matsText.substr(matStart) : matsText.substr(matStart, matEnd - matStart);

            if (auto matOpt = ParseCubeItem(matToken, ','); matOpt.has_value()) {
                recipes[currentRecipeIdx].materials.push_back(*matOpt);
            }

            if (matEnd == std::string_view::npos) {
                break;
            }
            matStart = matEnd + 1;
        }

        currentRecipeIdx++;
        if (recipeEnd == std::string_view::npos) {
            break;
        }
        recipeStart = recipeEnd + 1;
    }
}

bool CubeCraftHandler::IsOpen() const {
    return isOpen;
}

const std::vector<CubeRecipe>& CubeCraftHandler::GetRecipes() const {
    return recipes;
}

std::optional<CubeItem> CubeCraftHandler::ParseCubeItem(std::string_view token, char separator) const {
    size_t sepPos = token.find(separator);
    if (sepPos == std::string_view::npos) {
        return std::nullopt;
    }

    std::string_view vnumStr = token.substr(0, sepPos);
    std::string_view countStr = token.substr(sepPos + 1);

    CubeItem item{0, 0};
    auto vnumRes = std::from_chars(vnumStr.data(), vnumStr.data() + vnumStr.size(), item.vnum);
    auto countRes = std::from_chars(countStr.data(), countStr.data() + countStr.size(), item.count);

    if (vnumRes.ec == std::errc{} && countRes.ec == std::errc{}) {
        return item;
    }

    return std::nullopt;
}

} // namespace Handlers
} // namespace Network

#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <span>

namespace Domain {

/**
 * @brief Represents a single affect (buff/debuff) applied to an actor.
 */
struct AffectElement {
    uint32_t type;               /**< The unique identifier or category for the affect type. */
    uint8_t pointIndexApplyOn;   /**< The index of the specific character attribute or point this affect modifies. */
    int32_t applyValue;          /**< The magnitude or value applied by the affect. */
    uint32_t flag;               /**< Bitmask flag for the affect's visual or logic states. */
    int32_t duration;            /**< Remaining duration of the affect in milliseconds (or ticks). */
    int32_t spCost;              /**< The SP cost associated with maintaining or applying this affect. */
};

/**
 * @brief A modern C++20 container model for managing active affects (buffs/debuffs) on a character.
 * 
 * This class is strictly decoupled from the UI layer and is solely responsible for 
 * managing the memory state of affects based on the provided network payloads.
 */
class AffectContainerModel {
public:
    /**
     * @brief Constructs an empty affect container model.
     */
    AffectContainerModel() = default;

    /**
     * @brief Destroys the affect container model.
     */
    ~AffectContainerModel() = default;

    /**
     * @brief Clears all active affects from the container.
     */
    void Clear() {
        m_affects.clear();
    }

    /**
     * @brief Adds a new affect or updates an existing one if the type matches.
     * 
     * @param affect The affect element to add or update.
     */
    void AddAffect(const AffectElement& affect) {
        for (auto& existingAffect : m_affects) {
            if (existingAffect.type == affect.type && existingAffect.pointIndexApplyOn == affect.pointIndexApplyOn) {
                existingAffect = affect;
                return;
            }
        }
        m_affects.push_back(affect);
    }

    /**
     * @brief Removes an affect based on its type and application point.
     * 
     * @param type The type of the affect to remove.
     * @param pointIndexApplyOn The application point index of the affect to remove.
     * @return true if the affect was found and removed, false otherwise.
     */
    bool RemoveAffect(uint32_t type, uint8_t pointIndexApplyOn) {
        for (auto it = m_affects.begin(); it != m_affects.end(); ++it) {
            if (it->type == type && it->pointIndexApplyOn == pointIndexApplyOn) {
                m_affects.erase(it);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Retrieves an affect if it exists in the container.
     * 
     * @param type The type of the affect to find.
     * @param pointIndexApplyOn The application point index of the affect to find.
     * @return std::optional<AffectElement> containing the affect if found, or std::nullopt.
     */
    std::optional<AffectElement> GetAffect(uint32_t type, uint8_t pointIndexApplyOn) const {
        for (const auto& affect : m_affects) {
            if (affect.type == type && affect.pointIndexApplyOn == pointIndexApplyOn) {
                return affect;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Retrieves a read-only span of all current affects.
     * 
     * @return std::span<const AffectElement> viewing the internal affect collection.
     */
    std::span<const AffectElement> GetAllAffects() const {
        return m_affects;
    }

    /**
     * @brief Populates the container using a binary buffer representing a sequence of AffectElements.
     * 
     * @param buffer A span over the binary data representing an array of affects.
     * @return true if the buffer size is valid (a multiple of sizeof(AffectElement)), false otherwise.
     */
    bool LoadFromBuffer(std::span<const uint8_t> buffer) {
        if (buffer.size() % sizeof(AffectElement) != 0) {
            return false;
        }

        size_t count = buffer.size() / sizeof(AffectElement);
        m_affects.clear();
        m_affects.reserve(count);

        const AffectElement* elements = reinterpret_cast<const AffectElement*>(buffer.data());
        for (size_t i = 0; i < count; ++i) {
            m_affects.push_back(elements[i]);
        }

        return true;
    }

private:
    std::vector<AffectElement> m_affects; /**< The collection of currently active affects. */
};

} // namespace Domain

#pragma once

#include <cstdint>
#include <string_view>
#include <stdexcept>

/**
 * @brief Represents the available player-versus-player combat modes.
 */
enum class PvpMode : uint8_t {
    Peace = 0,
    Revenge = 1,
    Free = 2,
    Protect = 3,
    Guild = 4,
    MaxNum = 5
};

/**
 * @brief Manages the state of the PvP combat mode for an actor or client.
 */
class PvpModeState {
public:
    /**
     * @brief Constructs a new PvpModeState with default Peace mode.
     */
    PvpModeState() = default;

    /**
     * @brief Sets the current PvP mode.
     * 
     * @param mode The new PvP mode to set.
     * @return true if the mode was successfully changed, false if the mode was already set.
     * @throws std::out_of_range if the provided mode is invalid (>= MaxNum).
     */
    bool SetMode(PvpMode mode) {
        if (mode >= PvpMode::MaxNum) {
            throw std::out_of_range("Invalid PvP mode provided.");
        }
        
        if (currentMode == mode) {
            return false;
        }
        
        currentMode = mode;
        return true;
    }

    /**
     * @brief Gets the current PvP mode.
     * 
     * @return The currently active PvP mode.
     */
    [[nodiscard]] PvpMode GetMode() const noexcept {
        return currentMode;
    }

    /**
     * @brief Retrieves a string representation of the current PvP mode.
     * 
     * @return A std::string_view representing the name of the mode.
     */
    [[nodiscard]] std::string_view GetModeName() const noexcept {
        return GetModeName(currentMode);
    }

    /**
     * @brief Retrieves a string representation of a specific PvP mode.
     * 
     * @param mode The PvP mode to get the name for.
     * @return A std::string_view representing the name of the mode.
     */
    [[nodiscard]] static std::string_view GetModeName(PvpMode mode) noexcept {
        switch (mode) {
            case PvpMode::Peace: return "Peace";
            case PvpMode::Revenge: return "Revenge";
            case PvpMode::Free: return "Free";
            case PvpMode::Protect: return "Protect";
            case PvpMode::Guild: return "Guild";
            default: return "Unknown";
        }
    }

    /**
     * @brief Checks if the current mode allows attacking a target with the given mode.
     * 
     * @param targetMode The PvP mode of the target.
     * @return true if combat is permitted, false otherwise.
     */
    [[nodiscard]] bool CanAttack(PvpMode targetMode) const noexcept {
        if (currentMode == PvpMode::Peace) {
            return false;
        }
        
        if (currentMode == PvpMode::Free) {
            return true;
        }
        
        if (currentMode == PvpMode::Guild && targetMode != PvpMode::Protect) {
            return true;
        }
        
        if (currentMode == PvpMode::Revenge) {
            return true;
        }
        
        return false;
    }

private:
    PvpMode currentMode{PvpMode::Peace};
};

#include "Result.h"

namespace Client::Core {

[[nodiscard]] std::string_view to_string(PacketError error) noexcept {
    switch (error) {
        case PacketError::BufferUnderflow: return "PacketError::BufferUnderflow - Not enough data in buffer";
        case PacketError::InvalidHeader: return "PacketError::InvalidHeader - Header does not match expected format";
        case PacketError::ChecksumMismatch: return "PacketError::ChecksumMismatch - Checksum validation failed";
        case PacketError::VidMismatch: return "PacketError::VidMismatch - Target Entity ID (VID) mismatch";
        case PacketError::Timeout: return "PacketError::Timeout - Wait for packet timed out";
        default: return "PacketError::Unknown";
    }
}

[[nodiscard]] std::string_view to_string(EntityError error) noexcept {
    switch (error) {
        case EntityError::NotFound: return "EntityError::NotFound - The specified entity could not be found";
        case EntityError::AlreadyExists: return "EntityError::AlreadyExists - The entity already exists";
        case EntityError::Dead: return "EntityError::Dead - The entity is currently dead";
        case EntityError::OutOfRange: return "EntityError::OutOfRange - The entity is out of valid range";
        default: return "EntityError::Unknown";
    }
}

[[nodiscard]] std::string_view to_string(MountError error) noexcept {
    switch (error) {
        case MountError::NoHorseInstance: return "MountError::NoHorseInstance - The current character is not riding a horse/mount";
        case MountError::MotionKeyNotFound: return "MountError::MotionKeyNotFound - Mount motion key was not found in registry";
        case MountError::InvalidState: return "MountError::InvalidState - The mount is in an invalid state for this operation";
        default: return "MountError::Unknown";
    }
}

} // namespace Client::Core

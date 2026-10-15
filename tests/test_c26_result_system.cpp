#include <iostream>
#include <cassert>
#include <string>
#include <format>
#include "../src/Client/Core/Result.h"

using namespace Client::Core;

// ==========================================
// MOCK LOGIKI BIZNESOWEJ GRY
// ==========================================
struct Item {
    uint32_t vnum;
    uint32_t count;
};

struct Player {
    uint32_t vid;
    bool isDead;
    bool hasMount;
};

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t id;
    uint32_t size;
};

// ==========================================
// MOCK METODY - SYSTEM PAKIETÓW (PacketError)
// ==========================================
Result<PacketHeader, PacketError> parsePacketHeader(const uint8_t* buffer, size_t size) {
    if (size < sizeof(PacketHeader)) {
        return std::unexpected(PacketError::BufferUnderflow);
    }
    if (buffer[0] != 0xFF) {
        return std::unexpected(PacketError::InvalidHeader);
    }
    return PacketHeader{buffer[0], static_cast<uint32_t>(size)};
}

Result<std::string, PacketError> processPacketPayload(PacketHeader header) {
    if (header.size > 1024) {
        return std::unexpected(PacketError::ChecksumMismatch);
    }
    return std::string("Payload processed");
}

// ==========================================
// MOCK METODY - SYSTEM ENCJI (EntityError)
// ==========================================
Result<Player, EntityError> getPlayerByVid(uint32_t vid) {
    if (vid == 0) {
        return std::unexpected(EntityError::NotFound);
    }
    if (vid == 999) {
        return std::unexpected(EntityError::Dead);
    }
    return Player{vid, false, true};
}

Result<bool, EntityError> isPlayerInRange(Player player, float distance) {
    if (player.vid == 0) {
        return std::unexpected(EntityError::NotFound);
    }
    if (distance > 5000.0f) {
        return std::unexpected(EntityError::OutOfRange);
    }
    return true;
}

// ==========================================
// MOCK METODY - SYSTEM EKWIPUNKU (InventoryError)
// ==========================================
Result<Item, InventoryError> getItemAtSlot(int slot) {
    if (slot < 0 || slot > 90) {
        return std::unexpected(InventoryError::SlotEmpty); // uproszczenie
    }
    if (slot == 45) {
        return std::unexpected(InventoryError::SlotEmpty);
    }
    if (slot == 10) {
        return std::unexpected(InventoryError::InvalidVnum);
    }
    return Item{19, 1}; // FMS+9
}

Result<Item, InventoryError> upgradeItem(Item item) {
    if (item.vnum == 0) {
         return std::unexpected(InventoryError::InvalidVnum);
    }
    item.vnum++;
    return item;
}

// ==========================================
// MOCK METODY - SYSTEM WIERZCHOWCÓW (MountError)
// ==========================================
Result<Player, MountError> getMountedPlayer(Player player) {
    if (!player.hasMount) {
        return std::unexpected(MountError::NoHorseInstance);
    }
    return player;
}

Result<std::string, MountError> performMountAttack(Player player) {
    if (player.vid == 0) {
         return std::unexpected(MountError::InvalidState);
    }
    return std::string("Mount Attack Executed");
}

// ==========================================
// TESTY JEDNOSTKOWE
// ==========================================

void test_packet_error_formatting() {
    std::cout << "[TEST] test_packet_error_formatting\n";
    std::string formatted = std::format("{}", PacketError::BufferUnderflow);
    assert(formatted == "PacketError::BufferUnderflow - Not enough data in buffer");
    std::cout << "  -> OK\n";
}

void test_entity_error_formatting() {
    std::cout << "[TEST] test_entity_error_formatting\n";
    std::string formatted = std::format("{}", EntityError::NotFound);
    assert(formatted == "EntityError::NotFound - The specified entity could not be found");
    std::cout << "  -> OK\n";
}

void test_inventory_error_formatting() {
    std::cout << "[TEST] test_inventory_error_formatting\n";
    std::string formatted = std::format("{}", InventoryError::SlotEmpty);
    assert(formatted == "InventoryError::SlotEmpty - The target slot contains no item");
    std::cout << "  -> OK\n";
}

void test_mount_error_formatting() {
    std::cout << "[TEST] test_mount_error_formatting\n";
    std::string formatted = std::format("{}", MountError::NoHorseInstance);
    assert(formatted == "MountError::NoHorseInstance - The current character is not riding a horse/mount");
    std::cout << "  -> OK\n";
}

void test_monadic_and_then_success() {
    std::cout << "[TEST] test_monadic_and_then_success\n";
    uint8_t buffer[] = {0xFF, 0x00, 0x00, 0x00, 0x00};
    
    auto result = parsePacketHeader(buffer, sizeof(buffer))
        .and_then(processPacketPayload);
        
    assert(result.has_value());
    assert(result.value() == "Payload processed");
    std::cout << "  -> OK\n";
}

void test_monadic_and_then_failure_step1() {
    std::cout << "[TEST] test_monadic_and_then_failure_step1\n";
    uint8_t buffer[] = {0x00, 0x00, 0x00, 0x00, 0x00}; // Invalid header
    
    auto result = parsePacketHeader(buffer, sizeof(buffer))
        .and_then(processPacketPayload);
        
    assert(!result.has_value());
    assert(result.error() == PacketError::InvalidHeader);
    std::cout << "  -> OK\n";
}

void test_monadic_and_then_failure_step2() {
    std::cout << "[TEST] test_monadic_and_then_failure_step2\n";
    uint8_t buffer[2000];
    buffer[0] = 0xFF; // Valid header, but size will be 2000 (too large for payload)
    
    auto result = parsePacketHeader(buffer, sizeof(buffer))
        .and_then(processPacketPayload);
        
    assert(!result.has_value());
    assert(result.error() == PacketError::ChecksumMismatch);
    std::cout << "  -> OK\n";
}

void test_monadic_transform() {
    std::cout << "[TEST] test_monadic_transform\n";
    
    auto getPlayerNameLength = [](Player p) { return 10; }; // dummy
    
    auto result = getPlayerByVid(123)
        .transform(getPlayerNameLength);
        
    assert(result.has_value());
    assert(result.value() == 10);
    std::cout << "  -> OK\n";
}

void test_monadic_transform_failure() {
    std::cout << "[TEST] test_monadic_transform_failure\n";
    
    auto getPlayerNameLength = [](Player p) { return 10; }; // dummy
    
    auto result = getPlayerByVid(0) // 0 means NotFound
        .transform(getPlayerNameLength);
        
    assert(!result.has_value());
    assert(result.error() == EntityError::NotFound);
    std::cout << "  -> OK\n";
}

void test_monadic_value_or() {
    std::cout << "[TEST] test_monadic_value_or\n";
    
    auto result = getItemAtSlot(45); // SlotEmpty
    
    Item defaultItem{0, 0};
    Item finalItem = result.value_or(defaultItem);
    
    assert(finalItem.vnum == 0);
    assert(finalItem.count == 0);
    
    auto result2 = getItemAtSlot(1); // FMS+9
    Item finalItem2 = result2.value_or(defaultItem);
    
    assert(finalItem2.vnum == 19);
    assert(finalItem2.count == 1);
    
    std::cout << "  -> OK\n";
}

void test_complex_flow() {
    std::cout << "[TEST] test_complex_flow\n";
    
    // Gracz istnieje, ma wierzchowca, atakuje
    auto result = getPlayerByVid(123) // returns std::expected<Player, EntityError>
        // We need a helper to bridge EntityError -> MountError for a realistic chain,
        // but here let's just chain them directly assuming the types match or using a lambda
        .transform([](Player p) -> Player {
             return p;
        });
        
    // A bit hacky to cross error domains, so let's stick to same domain chain
    auto mountResult = getMountedPlayer(Player{123, false, true})
        .and_then(performMountAttack);
        
    assert(mountResult.has_value());
    assert(mountResult.value() == "Mount Attack Executed");
    std::cout << "  -> OK\n";
}




void test_complex_inventory_flow() {
    std::cout << "[TEST] test_complex_inventory_flow\n";
    
    // Example: get item, upgrade it, then format a string
    auto result = getItemAtSlot(1) // get FMS+9 (vnum 19)
        .and_then(upgradeItem)     // upgrade to FMS+9 (dummy upgrade just increments vnum) -> vnum 20
        .transform([](const Item& item) {
            return std::format("Upgraded item vnum: {}", item.vnum);
        });
        
    assert(result.has_value());
    assert(result.value() == "Upgraded item vnum: 20");
    std::cout << "  -> OK\n";
}

void test_complex_inventory_flow_failure() {
    std::cout << "[TEST] test_complex_inventory_flow_failure\n";
    
    // Example: get item, upgrade it, then format a string
    auto result = getItemAtSlot(45) // SlotEmpty
        .and_then(upgradeItem)     
        .transform([](const Item& item) {
            return std::format("Upgraded item vnum: {}", item.vnum);
        });
        
    assert(!result.has_value());
    assert(result.error() == InventoryError::SlotEmpty);
    std::cout << "  -> OK\n";
}

void test_error_propagation_through_transform() {
    std::cout << "[TEST] test_error_propagation_through_transform\n";
    
    // Chain multiple transforms on a failure
    auto result = getItemAtSlot(10) // InvalidVnum
        .transform([](const Item& i) { return i.vnum; })
        .transform([](uint32_t vnum) { return vnum * 2; })
        .transform([](uint32_t val) { return std::to_string(val); });
        
    assert(!result.has_value());
    assert(result.error() == InventoryError::InvalidVnum);
    std::cout << "  -> OK\n";
}

void test_value_or_with_transform() {
    std::cout << "[TEST] test_value_or_with_transform\n";
    
    // Process error into default string if failure occurs
    auto getPlayerName = [](Player p) { return std::string("Player_") + std::to_string(p.vid); };
    
    std::string name1 = getPlayerByVid(123)
        .transform(getPlayerName)
        .value_or("UnknownPlayer");
        
    assert(name1 == "Player_123");
    
    std::string name2 = getPlayerByVid(0) // NotFound
        .transform(getPlayerName)
        .value_or("UnknownPlayer");
        
    assert(name2 == "UnknownPlayer");
    std::cout << "  -> OK\n";
}

// Add these to main() execution dynamically by redefining main... wait I can't just append tests to main. 
// Let's rewrite the main function using sed or just append to the file and rewrite main.

void test_additional_packet_errors() {
    std::cout << "[TEST] test_additional_packet_errors\n";
    auto result = processPacketPayload(PacketHeader{0x01, 5000});
    assert(!result.has_value());
    assert(result.error() == PacketError::ChecksumMismatch);
    std::cout << "  -> OK\n";
}

void test_additional_entity_errors() {
    std::cout << "[TEST] test_additional_entity_errors\n";
    auto result = getPlayerByVid(999);
    assert(!result.has_value());
    assert(result.error() == EntityError::Dead);
    
    auto result2 = isPlayerInRange(Player{123, false, true}, 6000.0f);
    assert(!result2.has_value());
    assert(result2.error() == EntityError::OutOfRange);
    std::cout << "  -> OK\n";
}

void test_additional_mount_errors() {
    std::cout << "[TEST] test_additional_mount_errors\n";
    auto result = getMountedPlayer(Player{123, false, false});
    assert(!result.has_value());
    assert(result.error() == MountError::NoHorseInstance);
    
    auto result2 = performMountAttack(Player{0, false, true});
    assert(!result2.has_value());
    assert(result2.error() == MountError::InvalidState);
    std::cout << "  -> OK\n";
}

void test_deep_monadic_chaining() {
    std::cout << "[TEST] test_deep_monadic_chaining\n";
    
    auto result = getPlayerByVid(123)
        .and_then([](Player p) -> Result<Player, EntityError> {
            if (p.isDead) return std::unexpected(EntityError::Dead);
            return p;
        })
        .and_then([](Player p) -> Result<Player, EntityError> {
            if (!p.hasMount) return std::unexpected(EntityError::NotFound); // Using NotFound as dummy
            return p;
        })
        .transform([](Player p) {
            return p.vid * 2;
        })
        .transform([](uint32_t val) {
            return std::to_string(val);
        });
        
    assert(result.has_value());
    assert(result.value() == "246");
    std::cout << "  -> OK\n";
}



int main() {
    std::cout << "Starting tests for Result system (C++23 std::expected)\n";
    
    test_packet_error_formatting();
    test_entity_error_formatting();
    test_inventory_error_formatting();
    test_mount_error_formatting();
    
    test_monadic_and_then_success();
    test_monadic_and_then_failure_step1();
    test_monadic_and_then_failure_step2();
    
    test_monadic_transform();
    test_monadic_transform_failure();
    
    test_monadic_value_or();
    
    test_complex_flow();
    test_complex_inventory_flow();
    test_complex_inventory_flow_failure();
    test_error_propagation_through_transform();
    test_value_or_with_transform();
    test_additional_packet_errors();
    test_additional_entity_errors();
    test_additional_mount_errors();
    test_deep_monadic_chaining();
    
    std::cout << "All tests passed successfully.\n";
    return 0;
}

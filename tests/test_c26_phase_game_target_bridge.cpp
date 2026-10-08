#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdint>
#include <string>
#include <vector>
#include <span>
#include <expected>

// -------------------------------------------------------------
// Mock dependencies and system environment
// -------------------------------------------------------------
namespace EterBase {
    enum class PacketError {
        BufferUnderflow,
        InvalidHeader
    };
    template <typename T>
    using PacketResult = std::expected<T, PacketError>;
}

namespace Client::Network {
    struct TargetCreatePacket {
        int32_t lID;
        std::string szTargetName;
    };
    struct TargetUpdatePacket {
        int32_t lID;
        int32_t lX;
        int32_t lY;
    };
    struct TargetDeletePacket {
        int32_t lID;
    };
    struct CreateFlyPacket {
        uint8_t bType;
        uint32_t dwStartVID;
        uint32_t dwEndVID;
    };

    class TargetPacketCodec {
    public:
        static EterBase::PacketResult<TargetCreatePacket> DecodeTargetCreate(std::span<const uint8_t> buffer) {
            TargetCreatePacket p{123, "TestTarget"};
            return p;
        }
        static EterBase::PacketResult<TargetUpdatePacket> DecodeTargetUpdate(std::span<const uint8_t> buffer) {
            TargetUpdatePacket p{456, 10, 20};
            return p;
        }
        static EterBase::PacketResult<TargetDeletePacket> DecodeTargetDelete(std::span<const uint8_t> buffer) {
            TargetDeletePacket p{789};
            return p;
        }
        static EterBase::PacketResult<CreateFlyPacket> DecodeCreateFly(std::span<const uint8_t> buffer) {
            CreateFlyPacket p{0, 1, 2};
            return p;
        }
    };
}

#ifndef PyObject
struct PyObject {};
#endif

class CGraphicThingInstance {
public:
    int id;
};

class CInstanceBase {
public:
    CGraphicThingInstance* GetGraphicThingInstancePtr() { return &m_thing; }
    CGraphicThingInstance m_thing;
};

class CPythonMiniMap {
public:
    static CPythonMiniMap& Instance() { static CPythonMiniMap inst; return inst; }
    void CreateTarget(int id, const char* name) { created_targets.push_back(id); }
    void UpdateTarget(int id, int x, int y) { updated_targets.push_back(id); }
    void DeleteTarget(int id) { deleted_targets.push_back(id); }

    std::vector<int> created_targets;
    std::vector<int> updated_targets;
    std::vector<int> deleted_targets;
};

class CPythonBackground {
public:
    static CPythonBackground& Instance() { static CPythonBackground inst; return inst; }
    void CreateTargetEffect(int id, int x, int y) { effect_created.push_back(id); }
    void DeleteTargetEffect(int id) { effect_deleted.push_back(id); }

    std::vector<int> effect_created;
    std::vector<int> effect_deleted;
};

class CPythonCharacterManager {
public:
    static CPythonCharacterManager& Instance() { static CPythonCharacterManager inst; return inst; }
    CInstanceBase* GetInstancePtr(uint32_t vid) { 
        if (vid == 0) return nullptr; 
        return &inst; 
    }
    CInstanceBase inst;
};

class CFlyingManager {
public:
    static CFlyingManager& Instance() { static CFlyingManager inst; return inst; }
    void CreateIndexedFly(uint8_t type, CGraphicThingInstance* start, CGraphicThingInstance* end) {
        fly_created = true;
    }
    bool fly_created = false;
};

class CPythonNetworkStream {
public:
    PyObject* GetPhaseWindow(int phase) { return nullptr; }
};

#define PHASE_WINDOW_GAME 0

PyObject* Py_BuildValue(const char* format, ...) { return nullptr; }
void PyCallClassMemberFunc(PyObject* obj, const char* func, PyObject* args) { }

struct TPacketGCTargetCreate {
    uint16_t header;
    uint16_t length;
    int32_t lID;
    char szTargetName[33];
};

struct TPacketGCTargetUpdate {
    uint16_t header;
    uint16_t length;
    int32_t lID;
    int32_t lX, lY;
};

struct TPacketGCTargetDelete {
    uint16_t header;
    uint16_t length;
    int32_t lID;
};

struct TPacketGCCreateFly {
    uint16_t header;
    uint16_t length;
    uint8_t bType;
    uint32_t dwStartVID;
    uint32_t dwEndVID;
};

class PhaseGameTargetBridge {
public:
	static bool HandleTargetCreate(CPythonNetworkStream* pStream, const TPacketGCTargetCreate& pack);
	static bool HandleTargetUpdate(CPythonNetworkStream* pStream, const TPacketGCTargetUpdate& pack);
	static bool HandleTargetDelete(CPythonNetworkStream* pStream, const TPacketGCTargetDelete& pack);
	static bool HandleCreateFly(CPythonNetworkStream* pStream, const TPacketGCCreateFly& pack);
};

// Include SUT after mock headers are defined to prevent fatal includes
// Since #include "StdAfx.h" in the cpp file will fail, we can trick the preprocessor.
// By defining the macros guarding the files we can skip them in the actual cpp.

#define TEST_MODE_DISABLE_STDAFX
#include "../src/UserInterface/PythonNetworkStreamPhaseGameTarget.cpp"

// -------------------------------------------------------------
// Tests
// -------------------------------------------------------------

TEST_CASE("PhaseGameTargetBridge - TargetCreate") {
    CPythonNetworkStream stream;
    TPacketGCTargetCreate pack{};
    bool res = PhaseGameTargetBridge::HandleTargetCreate(&stream, pack);
    CHECK(res == true);
    // Codec returns 123
    CHECK(CPythonMiniMap::Instance().created_targets.back() == 123);
}

TEST_CASE("PhaseGameTargetBridge - TargetUpdate") {
    CPythonNetworkStream stream;
    TPacketGCTargetUpdate pack{};
    bool res = PhaseGameTargetBridge::HandleTargetUpdate(&stream, pack);
    CHECK(res == true);
    // Codec returns 456
    CHECK(CPythonMiniMap::Instance().updated_targets.back() == 456);
    CHECK(CPythonBackground::Instance().effect_created.back() == 456);
}

TEST_CASE("PhaseGameTargetBridge - TargetDelete") {
    CPythonNetworkStream stream;
    TPacketGCTargetDelete pack{};
    bool res = PhaseGameTargetBridge::HandleTargetDelete(&stream, pack);
    CHECK(res == true);
    // Codec returns 789
    CHECK(CPythonMiniMap::Instance().deleted_targets.back() == 789);
    CHECK(CPythonBackground::Instance().effect_deleted.back() == 789);
}

TEST_CASE("PhaseGameTargetBridge - CreateFly") {
    CPythonNetworkStream stream;
    TPacketGCCreateFly pack{};
    bool res = PhaseGameTargetBridge::HandleCreateFly(&stream, pack);
    CHECK(res == true);
    CHECK(CFlyingManager::Instance().fly_created == true);
}

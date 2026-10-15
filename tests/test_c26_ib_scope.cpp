#include <gtest/gtest.h>

// Mock CStateManager and STATEMANAGER
struct MockStateManager {
    int saveIndicesCalls = 0;
    int restoreIndicesCalls = 0;
    void* lastSavedIndexData = nullptr;
    unsigned int lastSavedBaseVertexIndex = 0;

    void SaveIndices(void* pIndexData, unsigned int BaseVertexIndex) {
        saveIndicesCalls++;
        lastSavedIndexData = pIndexData;
        lastSavedBaseVertexIndex = BaseVertexIndex;
    }

    void RestoreIndices() {
        restoreIndicesCalls++;
    }

    void Reset() {
        saveIndicesCalls = 0;
        restoreIndicesCalls = 0;
        lastSavedIndexData = nullptr;
        lastSavedBaseVertexIndex = 0;
    }
};

MockStateManager STATEMANAGER;

// Copy the implementation of IndexBufferScope directly to test it.
// We cannot easily test the header file since it hard-includes StdAfx.h which
// includes windows.h and d3d9.h, which we don't have available on the gtest build server.

typedef void* LPDIRECT3DINDEXBUFFER9;
typedef unsigned int UINT;

namespace EterLib::Render
{
    class IndexBufferScope
    {
    public:
        IndexBufferScope(LPDIRECT3DINDEXBUFFER9 pIndexData, UINT BaseVertexIndex)
        {
            STATEMANAGER.SaveIndices(pIndexData, BaseVertexIndex);
        }

        ~IndexBufferScope()
        {
            STATEMANAGER.RestoreIndices();
        }

        // Prevent copying and assignment
        IndexBufferScope(const IndexBufferScope&) = delete;
        IndexBufferScope& operator=(const IndexBufferScope&) = delete;
        
        // Prevent moving
        IndexBufferScope(IndexBufferScope&&) = delete;
        IndexBufferScope& operator=(IndexBufferScope&&) = delete;
    };
}

class IndexBufferScopeTest : public ::testing::Test {
protected:
    void SetUp() override {
        STATEMANAGER.Reset();
    }
};

TEST_F(IndexBufferScopeTest, BasicScope) {
    int dummyBufferData = 0;
    LPDIRECT3DINDEXBUFFER9 pBuffer = &dummyBufferData;
    UINT baseVertexIndex = 42;

    EXPECT_EQ(STATEMANAGER.saveIndicesCalls, 0);
    EXPECT_EQ(STATEMANAGER.restoreIndicesCalls, 0);

    {
        EterLib::Render::IndexBufferScope scope(pBuffer, baseVertexIndex);

        EXPECT_EQ(STATEMANAGER.saveIndicesCalls, 1);
        EXPECT_EQ(STATEMANAGER.lastSavedIndexData, pBuffer);
        EXPECT_EQ(STATEMANAGER.lastSavedBaseVertexIndex, baseVertexIndex);
        EXPECT_EQ(STATEMANAGER.restoreIndicesCalls, 0);
    }

    EXPECT_EQ(STATEMANAGER.restoreIndicesCalls, 1);
}

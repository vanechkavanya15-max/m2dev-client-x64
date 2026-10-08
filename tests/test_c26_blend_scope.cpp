#include <gtest/gtest.h>
#include <vector>
#include <utility>

// Define our mock CStateManager so we can intercept calls in the inline methods
// of BlendStateScope. We need to make sure we don't include the real CStateManager.h
#define __CSTATEMANAGER_H
#include <d3d9.h>

class CStateManager {
public:
    static CStateManager& Instance() {
        static CStateManager instance;
        return instance;
    }

    void SaveRenderState(D3DRENDERSTATETYPE type, DWORD value) {
        saveCalls.push_back({type, value});
    }

    void RestoreRenderState(D3DRENDERSTATETYPE type) {
        restoreCalls.push_back(type);
    }

    void Clear() {
        saveCalls.clear();
        restoreCalls.clear();
    }

    std::vector<std::pair<D3DRENDERSTATETYPE, DWORD>> saveCalls;
    std::vector<D3DRENDERSTATETYPE> restoreCalls;
};
#define STATEMANAGER (CStateManager::Instance())

#include "../src/EterLib/Render/RenderStateTypes.h"
#include "../src/EterLib/Render/BlendStateScope.h"

using namespace EterLib::Render;

class BlendStateScopeTest : public ::testing::Test {
protected:
    void SetUp() override {
        CStateManager::Instance().Clear();
    }
};

TEST_F(BlendStateScopeTest, OpaqueModeSetsCorrectStates) {
    {
        BlendStateScope scope(BlendMode::Opaque);
        auto& sm = CStateManager::Instance();
        
        ASSERT_EQ(sm.saveCalls.size(), 4);
        EXPECT_EQ(sm.saveCalls[0].first, D3DRS_ALPHABLENDENABLE);
        EXPECT_EQ(sm.saveCalls[0].second, FALSE);
        EXPECT_EQ(sm.saveCalls[1].first, D3DRS_SRCBLEND);
        EXPECT_EQ(sm.saveCalls[1].second, D3DBLEND_ONE);
        EXPECT_EQ(sm.saveCalls[2].first, D3DRS_DESTBLEND);
        EXPECT_EQ(sm.saveCalls[2].second, D3DBLEND_ZERO);
        EXPECT_EQ(sm.saveCalls[3].first, D3DRS_BLENDOP);
        EXPECT_EQ(sm.saveCalls[3].second, D3DBLENDOP_ADD);
        
        EXPECT_TRUE(sm.restoreCalls.empty());
    }
    
    auto& sm = CStateManager::Instance();
    ASSERT_EQ(sm.restoreCalls.size(), 4);
    EXPECT_EQ(sm.restoreCalls[0], D3DRS_BLENDOP);
    EXPECT_EQ(sm.restoreCalls[1], D3DRS_DESTBLEND);
    EXPECT_EQ(sm.restoreCalls[2], D3DRS_SRCBLEND);
    EXPECT_EQ(sm.restoreCalls[3], D3DRS_ALPHABLENDENABLE);
}

TEST_F(BlendStateScopeTest, AlphaBlendModeSetsCorrectStates) {
    {
        BlendStateScope scope(BlendMode::AlphaBlend);
        auto& sm = CStateManager::Instance();
        
        ASSERT_EQ(sm.saveCalls.size(), 4);
        EXPECT_EQ(sm.saveCalls[0].first, D3DRS_ALPHABLENDENABLE);
        EXPECT_EQ(sm.saveCalls[0].second, TRUE);
        EXPECT_EQ(sm.saveCalls[1].first, D3DRS_SRCBLEND);
        EXPECT_EQ(sm.saveCalls[1].second, D3DBLEND_SRCALPHA);
        EXPECT_EQ(sm.saveCalls[2].first, D3DRS_DESTBLEND);
        EXPECT_EQ(sm.saveCalls[2].second, D3DBLEND_INVSRCALPHA);
        EXPECT_EQ(sm.saveCalls[3].first, D3DRS_BLENDOP);
        EXPECT_EQ(sm.saveCalls[3].second, D3DBLENDOP_ADD);
    }
}

TEST_F(BlendStateScopeTest, AdditiveModeSetsCorrectStates) {
    {
        BlendStateScope scope(BlendMode::Additive);
        auto& sm = CStateManager::Instance();
        
        ASSERT_EQ(sm.saveCalls.size(), 4);
        EXPECT_EQ(sm.saveCalls[0].first, D3DRS_ALPHABLENDENABLE);
        EXPECT_EQ(sm.saveCalls[0].second, TRUE);
        EXPECT_EQ(sm.saveCalls[1].first, D3DRS_SRCBLEND);
        EXPECT_EQ(sm.saveCalls[1].second, D3DBLEND_SRCALPHA);
        EXPECT_EQ(sm.saveCalls[2].first, D3DRS_DESTBLEND);
        EXPECT_EQ(sm.saveCalls[2].second, D3DBLEND_ONE);
        EXPECT_EQ(sm.saveCalls[3].first, D3DRS_BLENDOP);
        EXPECT_EQ(sm.saveCalls[3].second, D3DBLENDOP_ADD);
    }
}

TEST_F(BlendStateScopeTest, MultiplyModeSetsCorrectStates) {
    {
        BlendStateScope scope(BlendMode::Multiply);
        auto& sm = CStateManager::Instance();
        
        ASSERT_EQ(sm.saveCalls.size(), 4);
        EXPECT_EQ(sm.saveCalls[0].first, D3DRS_ALPHABLENDENABLE);
        EXPECT_EQ(sm.saveCalls[0].second, TRUE);
        EXPECT_EQ(sm.saveCalls[1].first, D3DRS_SRCBLEND);
        EXPECT_EQ(sm.saveCalls[1].second, D3DBLEND_ZERO);
        EXPECT_EQ(sm.saveCalls[2].first, D3DRS_DESTBLEND);
        EXPECT_EQ(sm.saveCalls[2].second, D3DBLEND_SRCCOLOR);
        EXPECT_EQ(sm.saveCalls[3].first, D3DRS_BLENDOP);
        EXPECT_EQ(sm.saveCalls[3].second, D3DBLENDOP_ADD);
    }
}

TEST_F(BlendStateScopeTest, CustomConstructorSetsCorrectStates) {
    {
        BlendStateScope scope(true, D3DBLEND_INVDESTALPHA, D3DBLEND_DESTCOLOR, D3DBLENDOP_SUBTRACT);
        auto& sm = CStateManager::Instance();
        
        ASSERT_EQ(sm.saveCalls.size(), 4);
        EXPECT_EQ(sm.saveCalls[0].first, D3DRS_ALPHABLENDENABLE);
        EXPECT_EQ(sm.saveCalls[0].second, TRUE);
        EXPECT_EQ(sm.saveCalls[1].first, D3DRS_SRCBLEND);
        EXPECT_EQ(sm.saveCalls[1].second, D3DBLEND_INVDESTALPHA);
        EXPECT_EQ(sm.saveCalls[2].first, D3DRS_DESTBLEND);
        EXPECT_EQ(sm.saveCalls[2].second, D3DBLEND_DESTCOLOR);
        EXPECT_EQ(sm.saveCalls[3].first, D3DRS_BLENDOP);
        EXPECT_EQ(sm.saveCalls[3].second, D3DBLENDOP_SUBTRACT);
    }
}

TEST_F(BlendStateScopeTest, RestoresInReverseOrder) {
    {
        BlendStateScope scope(BlendMode::Opaque);
        // Do nothing else
    }
    
    auto& sm = CStateManager::Instance();
    // Restores must be exactly in reverse order of saves
    ASSERT_EQ(sm.saveCalls.size(), 4);
    ASSERT_EQ(sm.restoreCalls.size(), 4);
    
    EXPECT_EQ(sm.restoreCalls[0], sm.saveCalls[3].first); // D3DRS_BLENDOP
    EXPECT_EQ(sm.restoreCalls[1], sm.saveCalls[2].first); // D3DRS_DESTBLEND
    EXPECT_EQ(sm.restoreCalls[2], sm.saveCalls[1].first); // D3DRS_SRCBLEND
    EXPECT_EQ(sm.restoreCalls[3], sm.saveCalls[0].first); // D3DRS_ALPHABLENDENABLE
}

TEST_F(BlendStateScopeTest, NestedScopesBehavior) {
    {
        BlendStateScope outer(BlendMode::Opaque);
        {
            BlendStateScope inner(BlendMode::AlphaBlend);
            auto& sm = CStateManager::Instance();
            EXPECT_EQ(sm.saveCalls.size(), 8);
            EXPECT_TRUE(sm.restoreCalls.empty());
        }
        auto& sm = CStateManager::Instance();
        EXPECT_EQ(sm.saveCalls.size(), 8);
        EXPECT_EQ(sm.restoreCalls.size(), 4);
    }
    auto& sm = CStateManager::Instance();
    EXPECT_EQ(sm.restoreCalls.size(), 8);
}

TEST_F(BlendStateScopeTest, CustomConstructorDefaultOp) {
    {
        BlendStateScope scope(false, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
        auto& sm = CStateManager::Instance();
        
        ASSERT_EQ(sm.saveCalls.size(), 4);
        EXPECT_EQ(sm.saveCalls[0].first, D3DRS_ALPHABLENDENABLE);
        EXPECT_EQ(sm.saveCalls[0].second, FALSE);
        EXPECT_EQ(sm.saveCalls[1].first, D3DRS_SRCBLEND);
        EXPECT_EQ(sm.saveCalls[1].second, D3DBLEND_SRCALPHA);
        EXPECT_EQ(sm.saveCalls[2].first, D3DRS_DESTBLEND);
        EXPECT_EQ(sm.saveCalls[2].second, D3DBLEND_INVSRCALPHA);
        EXPECT_EQ(sm.saveCalls[3].first, D3DRS_BLENDOP);
        EXPECT_EQ(sm.saveCalls[3].second, D3DBLENDOP_ADD); // default
    }
}

TEST_F(BlendStateScopeTest, MultipleCustomConstructorsAndDestructors) {
    auto& sm = CStateManager::Instance();
    
    {
        BlendStateScope scope1(true, D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLENDOP_ADD);
        EXPECT_EQ(sm.saveCalls.size(), 4);
        
        {
            BlendStateScope scope2(false, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLENDOP_SUBTRACT);
            EXPECT_EQ(sm.saveCalls.size(), 8);
            
            {
                BlendStateScope scope3(BlendMode::Multiply);
                EXPECT_EQ(sm.saveCalls.size(), 12);
                EXPECT_EQ(sm.restoreCalls.size(), 0);
            }
            
            EXPECT_EQ(sm.restoreCalls.size(), 4);
            EXPECT_EQ(sm.restoreCalls[0], D3DRS_BLENDOP);
            EXPECT_EQ(sm.restoreCalls[1], D3DRS_DESTBLEND);
            EXPECT_EQ(sm.restoreCalls[2], D3DRS_SRCBLEND);
            EXPECT_EQ(sm.restoreCalls[3], D3DRS_ALPHABLENDENABLE);
        }
        
        EXPECT_EQ(sm.restoreCalls.size(), 8);
    }
    
    EXPECT_EQ(sm.restoreCalls.size(), 12);
}

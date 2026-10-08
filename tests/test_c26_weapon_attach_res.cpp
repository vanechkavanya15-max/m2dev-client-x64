#include <cassert>
#include <iostream>
#include <cstring>
#include "../src/EterLib/Render/WeaponAttachmentResolver.h"

int main() {
    EterLib::Render::WeaponAttachmentResolver resolver;
    D3DMATRIX bone, actor, local;
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            bone.m[i][j] = (i==j) ? 1.0f : 0.0f;
            actor.m[i][j] = (i==j) ? 1.0f : 0.0f;
            local.m[i][j] = (i==j) ? 1.0f : 0.0f;
        }
    }
    
    // Setup specific test values
    local.m[0][0] = 2.0f;
    bone.m[1][1] = 3.0f;
    actor.m[2][2] = 4.0f;
    
    D3DMATRIX result = resolver.ResolveAttachment(bone, actor, local);
    
    // Result should be local * bone * actor
    assert(result.m[0][0] == 2.0f);
    assert(result.m[1][1] == 3.0f);
    assert(result.m[2][2] == 4.0f);
    assert(result.m[3][3] == 1.0f);
    
    std::cout << "All WeaponAttachmentResolver tests passed!" << std::endl;
    return 0;
}


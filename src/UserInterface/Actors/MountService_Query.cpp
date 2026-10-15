#include "../StdAfx.h"
#include "IMountHorseService.h"
#include "../PythonCharacterManager.h"
#include "../InstanceBase.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"

namespace UserInterface::Actors {

class MountService final : public IMountHorseService {
public:
    void Mount(EterBase::EntityId riderId, uint32_t mountVnum) override {
        EterBase::ModernLogger::Error("MountService::Mount not implemented in query module.");
    }

    void Dismount(EterBase::EntityId riderId) override {
        EterBase::ModernLogger::Error("MountService::Dismount not implemented in query module.");
    }

    void Clear() override {
        EterBase::ModernLogger::Error("MountService::Clear not implemented in query module.");
    }

    bool IsMounted(EterBase::EntityId riderId) const override {
        auto* instance = CPythonCharacterManager::Instance().GetInstancePtr(riderId.value());
        if (!instance) {
            EterBase::ModernLogger::Warning("MountService::IsMounted: Actor with ID {} not found.", riderId.value());
            return false;
        }
        return instance->IsMountingHorse() != 0;
    }

    uint32_t GetMountVnum(EterBase::EntityId riderId) const override {
        EterBase::ModernLogger::Error("MountService::GetMountVnum not implemented in query module.");
        return 0;
    }
};

} // namespace UserInterface::Actors

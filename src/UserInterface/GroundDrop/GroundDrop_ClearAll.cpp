#include "../StdAfx.h"
#include "IGroundDropBatchRenderer.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../PythonItem.h"

namespace UserInterface::GroundDrop
{
    /**
     * @brief Zdarzenie oznaczajace wyczyszczenie wszystkich dropow z ziemi.
     */
    struct GroundDropsClearedEvent : public UserInterface::Core::IEvent
    {
    };

    /**
     * @brief Odizolowany modul odpowiedzialny za czyszczenie ziemi ze wszystkich przedmiotow.
     * Zgodnie z zasada SRP i zero-conflict, reszta metod interfejsu jest zaslepiona.
     */
    class GroundDropBatchRenderer_ClearAll : public IGroundDropBatchRenderer
    {
    public:
        virtual ~GroundDropBatchRenderer_ClearAll() = default;

        /**
         * @brief Usuwa wszystkie przedmioty upuszczone na ziemie.
         */
        void ClearAll() override
        {
            EterBase::ModernLogger::Info("GroundDropBatchRenderer_ClearAll: Trwa czyszczenie wszystkich dropow na ziemi.");
            
            // Czyszczenie wewnetrznego stanu C++ w core singletonie
            CPythonItem::Instance().DeleteAllItems();

            // Publikacja zdarzenia na szynie, aby w pelni oddzielic powiadamianie UI
            UserInterface::Core::EventBus::GetInstance().Publish(GroundDropsClearedEvent{});

            EterBase::ModernLogger::Info("GroundDropBatchRenderer_ClearAll: Drop wyczyszczony.");
        }

        EterBase::PacketResult<void> AddDrop(const GroundDropItemData& item) override
        {
            EterBase::ModernLogger::Error("AddDrop is not implemented in GroundDrop_ClearAll module.");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        EterBase::PacketResult<void> RemoveDrop(uint32_t virtualId) override
        {
            EterBase::ModernLogger::Error("RemoveDrop is not implemented in GroundDrop_ClearAll module.");
            return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }

        void UpdateDrops(float deltaTime) override
        {
            EterBase::ModernLogger::Error("UpdateDrops is not implemented in GroundDrop_ClearAll module.");
        }

        void RenderInstancedDrops() override
        {
            EterBase::ModernLogger::Error("RenderInstancedDrops is not implemented in GroundDrop_ClearAll module.");
        }

        bool IsInRangeToPick(uint32_t virtualId, float playerX, float playerY, float maxRange) const override
        {
            EterBase::ModernLogger::Error("IsInRangeToPick is not implemented in GroundDrop_ClearAll module.");
            return false;
        }
    };
}

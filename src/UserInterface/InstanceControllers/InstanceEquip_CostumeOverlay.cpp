#include "../StdAfx.h"
#include "IInstanceEquipmentModelController.h"
#include "EquipmentModelEvents.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <array>
#include <mutex>
#include <memory>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Kontroler modelu nakladajacy priorytety kostiumow nad zwykly ekwipunek.
     */
    class InstanceEquip_CostumeOverlay final : public IInstanceEquipmentModelController
    {
    public:
        InstanceEquip_CostumeOverlay()
        {
            m_parts.fill(EterBase::ItemVnum(0));
        }

        ~InstanceEquip_CostumeOverlay() override = default;

        /**
         * @brief Ustawia dany element modelu (Vnum).
         * @param part Czesc modelu (np. Main, CostumeBody, Weapon).
         * @param vnum Wirtualny numer przedmiotu.
         * @return Zwraca sukces lub blad.
         */
        EterBase::PacketResult<void> SetPart(ModelPart part, EterBase::ItemVnum vnum) override
        {
            auto partIndex = static_cast<size_t>(part);
            if (partIndex >= static_cast<size_t>(ModelPart::MaxParts))
            {
                EterBase::ModernLogger::Error("InstanceEquip_CostumeOverlay: Invalid ModelPart {}", partIndex);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            m_parts[partIndex] = vnum;
            EterBase::ModernLogger::Debug("InstanceEquip_CostumeOverlay: SetPart {} to Vnum {}", partIndex, vnum.value());
            
            Core::EventBus::GetInstance().Publish(EquipmentModelChangedEvent(part, vnum));
            return {};
        }

        /**
         * @brief Czysci (zeruje) dany element modelu.
         * @param part Czesc modelu.
         * @return Zwraca sukces lub blad.
         */
        EterBase::PacketResult<void> ClearPart(ModelPart part) override
        {
            auto partIndex = static_cast<size_t>(part);
            if (partIndex >= static_cast<size_t>(ModelPart::MaxParts))
            {
                EterBase::ModernLogger::Error("InstanceEquip_CostumeOverlay: ClearPart failed for invalid ModelPart {}", partIndex);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            m_parts[partIndex] = EterBase::ItemVnum(0);
            EterBase::ModernLogger::Debug("InstanceEquip_CostumeOverlay: ClearPart {}", partIndex);
            
            Core::EventBus::GetInstance().Publish(EquipmentModelChangedEvent(part, EterBase::ItemVnum(0)));
            return {};
        }

        /**
         * @brief Pobiera wirtualny numer czesci, wymuszajac wyswietlanie kostiumu jesli istnieje.
         * @param part Czesc modelu bazowego.
         * @return EterBase::ItemVnum wyswietlanej czesci.
         */
        EterBase::ItemVnum GetPartVnum(ModelPart part) const override
        {
            switch (part)
            {
                case ModelPart::Main:
                    if (m_parts[static_cast<size_t>(ModelPart::CostumeBody)].value() != 0)
                        return m_parts[static_cast<size_t>(ModelPart::CostumeBody)];
                    return m_parts[static_cast<size_t>(ModelPart::Main)];
                
                case ModelPart::Hair:
                    if (m_parts[static_cast<size_t>(ModelPart::CostumeHair)].value() != 0)
                        return m_parts[static_cast<size_t>(ModelPart::CostumeHair)];
                    return m_parts[static_cast<size_t>(ModelPart::Hair)];
                
                case ModelPart::Weapon:
                    if (m_parts[static_cast<size_t>(ModelPart::CostumeWeapon)].value() != 0)
                        return m_parts[static_cast<size_t>(ModelPart::CostumeWeapon)];
                    return m_parts[static_cast<size_t>(ModelPart::Weapon)];
                
                default:
                {
                    auto partIndex = static_cast<size_t>(part);
                    if (partIndex < static_cast<size_t>(ModelPart::MaxParts))
                        return m_parts[partIndex];
                    return EterBase::ItemVnum(0);
                }
            }
        }

        /**
         * @brief Ustawia poziom szczegolowosci dla modelu.
         * @param lodLevel Poziom LOD.
         */
        void SetLODLevel(uint8_t lodLevel) override
        {
            m_lodLevel = lodLevel;
            EterBase::ModernLogger::Debug("InstanceEquip_CostumeOverlay: Set LOD Level to {}", lodLevel);
        }

        /**
         * @brief Czysci wszystkie zapamietane elementy zbroi, kostiumow i broni.
         */
        void ClearAllParts() override
        {
            m_parts.fill(EterBase::ItemVnum(0));
            EterBase::ModernLogger::Debug("InstanceEquip_CostumeOverlay: Cleared all parts");
        }

    private:
        std::array<EterBase::ItemVnum, static_cast<size_t>(ModelPart::MaxParts)> m_parts;
        uint8_t m_lodLevel{0};
    };

    std::unique_ptr<IInstanceEquipmentModelController> CreateCostumeOverlayController()
    {
        return std::make_unique<InstanceEquip_CostumeOverlay>();
    }
}

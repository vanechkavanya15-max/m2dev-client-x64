#pragma once

#include "TransformComponentTable.h"
#include "CombatComponentTable.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../Packet.h"

namespace UserInterface::ECS
{
    /**
     * @brief Struktura na zwracaną pozycję.
     */
    struct PositionInfo
    {
        float x;
        float y;
        float z;
        float rotation;
    };

    /**
     * @brief Struktura na zwracane HP.
     */
    struct HpInfo
    {
        uint32_t currentHp;
        uint32_t maxHp;
    };

    /**
     * @brief Ultra-szybkie API zapytań dla innych podsystemów.
     */
    class ECSBridgeQueryApi
    {
    public:
        static ECSBridgeQueryApi& GetInstance();

        /**
         * @brief Pobiera pozycję encji.
         * @param table Referencja do tabeli TransformComponentTable
         * @param entityId Silny typ id encji
         * @return Oczekiwany wynik: PositionInfo lub EntityError
         */
        EterBase::Result<PositionInfo, EterBase::EntityError> GetPosition(
            const TransformComponentTable& table, 
            EterBase::EntityId entityId) const;

        /**
         * @brief Pobiera HP encji.
         * @param table Referencja do tabeli CombatComponentTable
         * @param entityId Silny typ id encji
         * @return Oczekiwany wynik: HpInfo lub EntityError
         */
        EterBase::Result<HpInfo, EterBase::EntityError> GetHp(
            const CombatComponentTable& table, 
            EterBase::EntityId entityId) const;

        /**
         * @brief Powiadamia o zmianie HP przez EventBus.
         */
        EterBase::PacketResult<void> NotifyHpUpdate(
            EterBase::EntityId entityId) const;
            
    private:
        ECSBridgeQueryApi() = default;
        ~ECSBridgeQueryApi() = default;
        ECSBridgeQueryApi(const ECSBridgeQueryApi&) = delete;
        ECSBridgeQueryApi& operator=(const ECSBridgeQueryApi&) = delete;
    };
}

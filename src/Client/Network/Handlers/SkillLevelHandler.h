#pragma once

#include "../../Gameplay/SkillDomain.h"
#include "../../../EterBase/Result.h"
#include <cstdint>
#include <functional>
#include <span>
#include <optional>

namespace Client::Network {

    /**
     * @class SkillLevelHandler
     * @brief Odpowiada za obsluge pakietu GC::SKILL_LEVEL z zachowaniem zasady Zero-Conflict.
     * 
     * Wykorzystuje zewnetrzny callback do tlumaczenia legacy indeksow na identyfikatory
     * domenowe, izolujac logike biznesowa od zaleznosci interfejsu (CPythonPlayer).
     */
    class SkillLevelHandler {
    public:
        using IndexToSkillIdMapper = std::function<std::optional<Client::Gameplay::SkillId>(uint8_t)>;

        /**
         * @brief Inicjalizuje handler referencja do domeny i funkcja tlumaczaca indeksy.
         * 
         * @param skillDomain Referencja do centralnego repozytorium stanow umiejetnosci.
         * @param mapper Callback mapujacy indeks pakietu (0-254) na Client::Gameplay::SkillId.
         */
        SkillLevelHandler(Client::Gameplay::SkillDomain& skillDomain, IndexToSkillIdMapper mapper);

        /**
         * @brief Parsuje pakiet i aktualizuje poziomy umiejetnosci w domenie.
         * 
         * @param payload Surowe dane z bufora sieciowego.
         * @return EterBase::PacketResult<void> Sukces (pusty) lub blad (np. BufferUnderflow).
         */
        EterBase::PacketResult<void> HandleSkillLevel(std::span<const uint8_t> payload);

    private:
        Client::Gameplay::SkillDomain& m_skillDomain;
        IndexToSkillIdMapper m_mapper;
    };

} // namespace Client::Network

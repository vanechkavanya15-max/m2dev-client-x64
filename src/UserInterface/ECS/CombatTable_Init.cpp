#include "../StdAfx.h"
#include "CombatComponentTable.h"
#include "../Packet.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../Core/EventBus.h"
#include "../Core/CombatInitEvent.h"

namespace UserInterface::ECS
{
    /**
     * @brief Inicjalizuje tabele komponentow walki z ustalona poczatkowa pojemnoscia.
     *
     * Funkcja czysci istniejacy stan tabeli, nastepnie rezerwuje pamiec 
     * aby uniknac realokacji w trakcie uzytkowania w symulacji (SoA)
     * zgodnie z wytycznymi bezposredniej odpowiedzialnosci encji ECS.
     * Publikuje takze zdarzenie inicjalizacyjne dla GUI, uzywajac EventBus.
     *
     * @param table Referencja do tabeli CombatComponentTable do zainicjalizowania.
     * @param capacity Ilosc encji do zarezerwowania w pamieci.
     * @return EterBase::PacketResult<void> Sukces inicjalizacji.
     */
    EterBase::PacketResult<void> EcsCombatInit(CombatComponentTable& table, size_t capacity)
    {
        table.Clear();
        table.Reserve(capacity);

        // Jako ze to inicjalizacja HP i walki, domyslnie stan moze byc wypelniany przez inne podsystemy.
        // Wytyczne C++23: Logowanie via LogModern
        EterBase::ModernLogger::Info("Zainicjalizowano CombatComponentTable z pojemnoscia: {}", capacity);

        // Wyslanie zdarzenia o inicjalizacji tabeli walki. Uzycie poprawnej nazwy GetInstance() 
        Core::EventBus::GetInstance().Publish(Core::Events::CombatTableInitializedEvent{capacity});

        return {};
    }
}

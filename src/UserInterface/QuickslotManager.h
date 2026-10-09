#pragma once

#include "GameType.h"
#include "Packet.h"
#include "Client/Gameplay/QuickslotDomain.h"
#include <array>
#include <functional>

/**
 * @brief Klasa zarzadzajaca paskami szybkiego dostepu (Quickslot) gracza w UserInterface.
 * 
 * Wydzielona z CPythonPlayer w celu redukcji monolitu i enkapsulacji logiki
 * stronicowania, operacji sieciowych oraz delegacji do QuickslotDomain.
 */
class QuickslotManager
{
public:
    using SkillClickHandler = std::function<void(DWORD)>;
    using EmotionActHandler = std::function<void(DWORD)>;
    using PageChangeHandler = std::function<void()>;

    QuickslotManager();
    ~QuickslotManager() = default;

    /**
     * @brief Resetuje caly stan paskow szybkiego dostepu.
     */
    void Clear() noexcept;

    /**
     * @brief Rejestracja delegatow zdarzen akcji ze slotow.
     */
    void SetSkillClickHandler(SkillClickHandler handler) { m_skillClickHandler = std::move(handler); }
    void SetEmotionActHandler(EmotionActHandler handler) { m_emotionActHandler = std::move(handler); }
    void SetPageChangeHandler(PageChangeHandler handler) { m_pageChangeHandler = std::move(handler); }

    // Dostepp do strony
    [[nodiscard]] int GetQuickPage() const;
    void SetQuickPage(int nQuickPageIndex);

    // Konwersja indeksow
    [[nodiscard]] DWORD LocalQuickSlotIndexToGlobalQuickSlotIndex(DWORD dwLocalSlotIndex) const;

    // Pobieranie danych slotow
    void GetGlobalQuickSlotData(DWORD dwGlobalSlotIndex, DWORD* pdwWndType, DWORD* pdwWndItemPos) const;
    void GetLocalQuickSlotData(DWORD dwSlotPos, DWORD* pdwWndType, DWORD* pdwWndItemPos) const;

    // Referencje do struktur TQuickSlot (kompatybilnosc z interfejsem CPythonPlayer)
    [[nodiscard]] TQuickSlot & RefLocalQuickSlot(int SlotIndex);
    [[nodiscard]] TQuickSlot & RefGlobalQuickSlot(int SlotIndex);

    // Operacje modyfikacji slotow (np. pakiety z serwera)
    void AddQuickSlot(int QuickslotIndex, char IconType, char IconPosition);
    void DeleteQuickSlot(int QuickslotIndex);
    void MoveQuickSlot(int Source, int Target);
    void RemoveQuickSlotByValue(int iType, int iPosition);

    // Zgloszenia pakietowe do serwera (zadania uzytkownika z UI)
    void RequestMoveGlobalQuickSlotToLocalQuickSlot(DWORD dwGlobalSrcSlotIndex, DWORD dwLocalDstSlotIndex);
    void RequestAddLocalQuickSlot(DWORD dwLocalSlotIndex, DWORD dwWndType, DWORD dwWndItemPos);
    void RequestAddToEmptyLocalQuickSlot(DWORD dwWndType, DWORD dwWndItemPos);
    void RequestDeleteGlobalQuickSlot(DWORD dwGlobalSlotIndex);
    void RequestUseLocalQuickSlot(DWORD dwLocalSlotIndex);

    // Dostep do lezacej nizej domeny Gameplay
    [[nodiscard]] Client::Gameplay::QuickslotDomain& GetDomain() noexcept;
    [[nodiscard]] const Client::Gameplay::QuickslotDomain& GetDomain() const noexcept;

private:
    std::array<TQuickSlot, QUICKSLOT_MAX_NUM> m_quickSlots{};

    SkillClickHandler m_skillClickHandler;
    EmotionActHandler m_emotionActHandler;
    PageChangeHandler m_pageChangeHandler;
};

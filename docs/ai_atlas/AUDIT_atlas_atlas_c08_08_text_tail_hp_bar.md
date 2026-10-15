---
task_id: "atlas_c08_08_text_tail_hp_bar"
cluster: "UI"
module_name: "Rzutowanie Paskow HP Bytow ze Swiata 3D na Ekran 2D"
target_files:
- src/UserInterface/InstanceControllers/InstanceTextTailLinker.cpp
- src/UserInterface/TextTail/TextTailHelper.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_08_text_tail_hp_bar.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Analizowany modul jest odpowiedzialny za proces rzutowania (projekcji) elementow interfejsu przypisanych do obiektow przestrzennych - w szczegolnosci paskow HP, poziomow, rang oraz nazw graczy/potworow (znanych ogolnie jako TextTail) - ze swiata 3D (World Space) na wspolrzedne 2D ekranu (Screen Space).
- **Pozycja w Petli Gry:** Kod ten wywolywany jest zazwyczaj w fazie OnUpdate lub przed OnRender w glownym watku gry, gdy pozycje wszystkich aktorow w 3D zostaly ustalone i macierze widoku sa gotowe, ale sam wlasciwy rendering okien UI (2D) jeszcze sie nie rozpoczal.
- **Data Flow & Control Flow:** Wspolrzedne pozycji podmiotow (w formacie 3D x,y,z) sa przeliczane przy pomocy zlaczonych macierzy widoku (View) i projekcji (Projection) do znormalizowanej przestrzeni (NDC). Nastepnie przy uzyciu przesuniecia dla srodka ekranu obliczane sa pozycje x, y oraz z-depth ekranowe.
- **Cykl zycia:** Podmioty sa rejestrowane do systemu (RegisterActorTail / RegisterItemTail) tworzac wewnetrzne stany `ScreenProjectionState`. Nastapuje regularna aktualizacja ich fizycznej pozycji, a po wyrejestrowaniu sa usuwane z globalnej mapy `m_tails`. Modul oparty o bezstanowe publikowanie wynikow jako eventy (EventBus).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** Kod wywolywany jest przez zarzadce instancji UI (`CPythonTextTail` lub nowoczesny zarzadca sceny). Wynik dzialania odbierany jest przez subskrybentow `UserInterface::Core::EventBus` nasluchujacych na `TextTailPositionUpdateEvent`.
- **Zaleznosci wyjsciowe (Outbound):** Modul powiadamia swiat zewnetrzny przez `EventBus`. Korzysta rowniez z `EterBase::ModernLogger` (np. do metody RenderBatch) oraz strukturalnego zglaszania bledow za pomoca `EterBase::PacketResult`.
- **Drzewo dyrektyw `#include`:** 
  - `../StdAfx.h` (Standardowy prekompilowany naglowek projektu)
  - `ITextTailService.h` (Interfejs dla przyslosciowych rozwiazan, wstrzykiwanie zaleznosci)
  - `EterBase/LogModern.h`, `EterBase/Result.h`, `EterBase/StrongTypes.h` (Zaleznosci fundamentalne, systemy narzedziowe)
  - `UserInterface/Core/EventBus.h` (Komunikacja miedzymodulowa bezposrednia)
  - Standardowe narzedzia C++ (`unordered_map`, `string`, `vector`). Brak bezposredniego ryzyka zaleznosci cyklicznych w obrebie domeny rzutowania.
- **Model pamieciowy:** W pelni oparty na alokacjach wartosciowych. Kontenery `std::unordered_map` obsluguja pamiec w sposob przezroczysty, obiekty przechowywane stosowo (RAII). Do ID instancji uzywane sa silnie typowane wartosci `EterBase::EntityId` dla maksymalnego bezpieczenstwa i czytelnosci. Brak nagich wskaznikow.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)
- **Tabela Klas i Struktur:**
  - `ITextTailService`: Interfejs abstrakcyjny silnika text-tail, bazuje na czystych funkcjach wirtualnych (=0).
  - `TextTailScreenProjection`: Konkretna implementacja narzedzia przeliczajacego matematyke rzutowania. Skupia operacje iteracji na mapach stanow.
  - `ScreenProjectionState` (Rozmiar ok. 28 bajtow): Kontener fizyki 3D/2D; zawiara float x, y, z (3D) oraz sx, sy, sz (2D), i booleana visible (widocznosc na ekranie).
  - `TextTailPositionUpdateEvent`: Event dziedziczacy z `IEvent`, 24-28 bajtow. Posiada EterBase::EntityId (vid) i parametry wspolrzednych.

- **Tabela Metod Publicznych (`TextTailScreenProjection`):**
  - `RegisterActorTail(const TextTailCreateData& data) -> EterBase::PacketResult<void>` - Pre-condition: Podmiot nie figuruje na liscie `m_tails`. Side effect: rezerwacja wezla w mapie.
  - `UpdateScreenPositions(float viewMatrix[16], float projMatrix[16]) -> void` - Czysta metoda matematyczna, dokonuje mnozenia wektorow przez macierze, wysyla eventy przez EventBus do reszty systemu.

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - `ScreenProjectionState`: 
    [0x00] float x
    [0x04] float y
    [0x08] float z
    [0x0C] float sx
    [0x10] float sy
    [0x14] float sz
    [0x18] bool visible
    Kluczowe dla narzedzi reverse engineering i hookowania - sztywny, plaski format bez wirtualnego wskaznika (vtable).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Integracja z systemem nastapi np. w obsludze `Packet_TargetHP` (plik `TargetHPHandler.cpp`), ktory synchronizuje wartosci poziomu zycia bytow. Same rzutowanie dotyczy wylacznie wyswietlania wynikow otrzymanych wczesniej przez pakiety protokolu `CG/GC`.
- **Metody Pythona (`PyMethodDef`):** Integracja posrednia - Python ui (`CPythonTextTail` w srodowisku C++) aktualizuje dane, m.in. korzystajac z klasycznego powiazania do wczesniejszego systemu Granny / PyAPI. System ekranowych rzutowan udostepnia dane subskrybentom eventow w swiecie C++.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Kod wymaga wywolywania wylacznie w stalym i przewidywalnym watku gry - ze wzgledu na brak mechanizmow `std::mutex` operowanie na mapie `m_tails` (rejestracja i rzutowanie jednoczesnie w wielu watkach) grozi naruszeniem pamieci i zrzutem rdzenia programu (segfault).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Metoda wewnetrzna bazuje na wspolrzednych 800x600 zaszytych bezposrednio w algorytmie (hardcoded). W nowoczesnym silniku jest to powazny punkt usterki skalowania responsywnego dla rozdzielczosci innych niz 4:3 na sztywnym wymiarze okna gry. 
- Algorytm posiada swietne zapobieganie dzieleniu przez zero w wektorach: `invW` i dzielenie zachodzi tylko wtedy, gdy `w > 0.01f`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** W przypadku, gdy nalezy rozszerzyc rzutowanie o np. skale paska (Z-depth scaling, im dalej, tym pasek jest mniejszy) nalezy uzyc wyliczonej wartosci `sz` jako modyfikatora scale factor na wyjsciowym obiekcie EventBus `TextTailPositionUpdateEvent`. Dodaj nowe pole do struktury eventu, i dokonaj wyliczenia interpolacji.
- **Jak debugowac i logowac:** Przelacz parametry logu w instancji `EterBase::ModernLogger::Info` podczas metody `RenderBatch`. Aby uzyc logowania precyzyjnego wektorow uzywaj wylacznie makr std::format (zgodnych z C++23).
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Modul jest silnie zaprojektowany zgodnie ze standardami odciecia testowego: brak bezposredniej koniecznosci wstrzykiwania sprzetu wideo. Mozesz napisac testy z wlasnymi mokami widoku i wektorami tozsamosci oraz nasluchiwac EventBus pod katem eventu `TextTailPositionUpdateEvent`.


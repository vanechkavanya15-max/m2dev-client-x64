---
task_id: "atlas_c08_07_text_tail_core"
cluster: "UI"
module_name: "CPythonTextTail - Menedzer Etykiet Tekstowych Nad Bytami"
target_files:
- src/UserInterface/PythonTextTail.cpp
- src/UserInterface/PythonTextTail.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_07_text_tail_core.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** `CPythonTextTail` (wzorzec Singleton) to glowne narzedzie renderujace dymki, nazwy graczy, gildii, poziomy, teksty nad upuszczonymi przedmiotami (lupem) w przestrzeni swiata gry, ktore "wiskaja" (text tail) za postacia lub przedmiotem.
- **Wywolywanie:** Metody aktualizujace pozycje etykiet (Update) i wyswietlania ich (Render) uzywane sa glownie w obrebie petli gry w warstwie GUI klienta i modulu 3D.
- **Przeplyw danych:** Metody z `PythonTextTailModule` eksportowane do Pythona wchodza w interakcje z systemem (np. ustawienie kolorow, zarzadzanie pokazywaniem nazw). Zaleznosc ze strukturami 3D zapewnia rzutowanie (projekcje) pozycji przestrzennej `CGraphicObjectInstance` na wspolrzedne ekranowe (`TPixelPosition`).
- **Cykl zycia obiektow:** Pola typu `STextTail` sa alokowane uzywajac puli pamieci `CDynamicPool<STextTail>`. Rejestrowane sa oddzielnie dla bytow i czatu, modyfikowane asynchronicznie, a przy zamknieciu lub ukryciu zasoby instancji tekstowych GUI zwalniaja zaalokowana pamiec.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul Pythona `PythonTextTailModule.cpp`, instancje postaci/postaci ze wschodnich (`InstanceVisualComponent`), efekty instancji (`InstanceBaseEffect`), sieciowy procesor czatu (`PythonNetworkStreamPhaseGameChat`), zarzadca gildii i przedmiotow upuszczonych (`PythonItem`).
- **Zaleznosci wyjsciowe (Outbound):** Komponenty silnika graficznego Metin2 (EterLib) tj. `CGraphicTextInstance`, `CGraphicMarkInstance`, `CGraphicObjectInstance`, moduly renderujace pozycje czcionek. System zarzadzania znacznikami gildii: `CGuildMarkManager`.
- **Drzewo dyrektyw `#include`:** Wewnatrz naglowka: `EterBase/Singleton.h`. Powinno zwracac uwage na ladowanie czcionek i interakcje z silnikiem tekstu z przestrzeni.
- **Model pamieciowy:** Czyste wskazniki uzywane sa glownie do etykiet i instancji czcionek, struktury `TTextTail` pobierane i zwracane przez `CDynamicPool`. Istnieje ryzyko "dangling pointers", zwlaszcza z wlascicielem etykiety (`CGraphicObjectInstance * pOwner`). Wymagany refaktoring na VirtualID do eliminacji usterki (jak mowi TODO w kodzie).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
    - `CPythonTextTail` (Singleton): Glowne api, zarzadzajace etykietami.
    - `STextTail`: Kluczowa struktura reprezentujaca etykiete tekstowa w przestrzeni, wlasciciel watku D3D. Zajmuje kilkadziesiat bajtow pamieci (glownie instancje `CGraphicTextInstance*`, offsety x/y/z float, D3DXCOLOR, zywotnosc, wysokosc itd.).
- **Tabela Metod Publicznych:**
    - `RegisterCharacterTextTail(DWORD dwGuildID, DWORD dwVirtualID, const D3DXCOLOR & c_rColor, float fAddHeight)`: Rejestruje tekst gracza.
    - `AttachLevel(DWORD dwVID, const char* c_szText, const D3DXCOLOR& c_rColor)`: Dodaje etykiete poziomu przed nazwa postaci.
    - `Render()`: Renderowanie tekstow na bufor.
    - `Pick(int ixMouse, int iyMouse)`: Funkcja wybierajaca obiekt pod kursorem myszy z bazy etykiet itemow (lupu).
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
    - W strukturze `STextTail` szczegolnie wazne sa zagniezdzone instancje CGraphic: `pTextInstance`, `pOwnerTextInstance`, `pMarkInstance`, `pGuildNameTextInstance`, `pTitleTextInstance`, `pLevelTextInstance`. Zmiany pozycji na `x, y, z` stanowia punkt do trackowania obiektu na ekranie.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul odbiera bezposrednie rzutowania od strony interfejsu sieciowego podczas czatu w grze np. via `CPythonNetworkStreamPhaseGameChat`.
- **Metody Pythona (`PyMethodDef`):** Eksportuje modul modulu "textTail". Zawiera funkcje:
    - `textTail.Clear()`, `textTail.UpdateAllTextTail()`, `textTail.Render()`, `textTail.ShowCharacterTextTail()`, `textTail.GetPosition()`, `textTail.IsChat()`, `textTail.ArrangeTextTail()`, `textTail.ShowAllTextTail()`, `textTail.Pick()`, `textTail.SelectItemName()`, `textTail.EnablePKTitle()`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie instancje i manipulacje musza byc wywolywane z watku glownego D3D (Main Thread). Uzycie w tle asynchronicznym wywola natychmiastowe problemy dostepowe (race condition) do pamieci renderingu tekstu.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Utrzymanie wskaznika `CGraphicObjectInstance * pOwner` (jest komentarz `Todo : To zrobic przez VID. Jesli postac zniknie w trakcie, moze zglosic blad.`). Oproznienie struktur pociaga potrzebe czyszczenia wszystkich powiazanych wezlow graficznych (Dangling pointers).
- **Zarzadzanie zasobami (RAII):** Pulowane przez `CDynamicPool`, lecz `DeleteTextTail` musi systematycznie kasowac dynamiczne obiekty `CGraphicTextInstance::Delete()`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji:** Ustaw deklaracje w `PythonTextTail.h` oraz metody pomocnicze, nastepnie w `PythonTextTail.cpp` zaimplementuj logike modyfikacji pozycji. Pamietaj o aktualizacji `Render()` lub zwolnieniu elementow w `DeleteTextTail`. Jesli funkcja musi byc kontrolowana z poziomu Python'a, zarejestruj ja w `initTextTail()` w `PythonTextTailModule.cpp`.
- **Jak debugowac i logowac:** Uzywaj makra `Tracef` / `Tracenf` do sledzenia zdarzen. Zwroc uwage na renderowanie bez update'a (niepoprawna rzutowanie matrixa na ekran). 
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Zbudowanie unit testu dla `CPythonTextTail` wymaga zmockowania calego podsystemu `CGraphicTextInstance` / czcionek, `CDynamicPool`, by weryfikowac operacje na `std::map<DWORD, TTextTail*>` poprzez same operatory VID.

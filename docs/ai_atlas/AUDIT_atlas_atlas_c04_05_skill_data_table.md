---
task_id: "atlas_c04_05_skill_data_table"
cluster: "CBT"
module_name: "CPythonSkill - Tabela Umiejetnosci i Baza Skilli"
target_files:
- src/UserInterface/PythonSkill.cpp
- src/UserInterface/PythonSkill.h
- src/Client/Gameplay/SkillDomain.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_05_skill_data_table.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CPythonSkill` dziala jako globalny wzorzec Singleton i odpowiada wylacznie za przechowywanie statycznych metadanych umiejetnosci. Ladowanie danych nastepuje na etapie inicjalizacji aplikacji z plikow tekstowych VFS (np. `skilltable.txt`, `skilldesc.txt`). Tabela przechowuje zdefiniowane wartosci uzywane pozniej m.in. przy renderowaniu interfejsu uzytkownika, wyswietlaniu ikon skilli, czy tez pobieraniu opisow i formul zapotrzebowania na mane (SP).

Nowoczesna architektura dzieli ta logike na dwie zupelnie odrebne warstwy:
1. `CPythonSkill` (w `UserInterface`) zarzadza wylacznie *statycznymi metadanymi* pobieranymi z wirtualnego systemu plikow.
2. `Client::Gameplay::SkillDomain` zarzadza *dynamiczna logika biznesowa* oraz *stanem umiejetnosci gracza* (np. przeliczanie faktycznego kosztu many na podstawie poziomu zaawansowania umiejetnosci, obsluga czasow odnawiania - cooldownow oraz status Toggle).

Przeplyw danych w `SkillDomain` nastepuje w glownej petli gry - wymaga on wywolywania metody `UpdateCooldowns()` w kazdym ticku, co pozwala na poprawne odnawianie sie umiejetnosci. Konstruktory zwalniajace nie dokonuja zlozonych dealokacji wielkich buforow (dane z klasycznej CPythonSkill przechodza na wektory i mapy).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Python (skrypty UI) wola metody zdefioniowane w `s_methods` przy tworzeniu interfejsu oraz obsludze interakcji.
- W in-game, obiekty `CInstanceBase` moga pytac `CPythonSkill` o dane konfiguracyjne.
- Zarzadzanie graczem pobiera informacje o cooldownach / kosztach many wola interfejs `SkillDomain`.

**Zaleznosci wyjsciowe (Outbound):**
- System wirtualnego systemu plikow (`PackManager`) do czytania tablic i opisow umiejetnosci.
- `EterBase::Poly` (wielomiany) sluzace do kalkulowania wartosci skalujacych sie z poziomem (koszt SP, sila razenia, czas bufa).
- Elementy graficzne pobieraja bezposrednio wskazniki na zasoby (ikonki dla GUI).
- `SkillDomain` emituje oraz posluguje sie `DomainErrors`, `DomainEvents`, w celach zarzadzania rezultatem rzucania czaru (`Result<void, SkillError>`).
- Biblioteka `<shared_mutex>` dla synchronizacji danych stanu umiejetnosci na wypadek wielowatkowego dostepu.

**Drzewo dyrektyw `#include`:**
- `PythonSkill.cpp` uzywa m.in. `EterBase/Poly/Poly.h`, `PackLib/PackManager.h`, `InstanceBase.h`, `PythonPlayer.h`.
- `SkillDomain.h` includuje m.in. `<shared_mutex>`, `SkillCooldownTracker.h`, `DomainErrors.h`, `DomainEvents.h`, `StrongTypes.h`.

**Model pamieciowy:**
- W CPythonSkill uzywane sa glownie standardowe kontenery `std::map` oraz `std::vector` ze zmiennymi typu `std::string` pod dane tablicowe. Uzywane sa `DWORD` i `BYTE`.
- Klasa SkillDomain polega na `std::unordered_map` przypisujac `SkillId` (zdefiniowane jako StrongType) do obiektow stanu umiejetnosci.
- Menedzer domenowy wykorzystuje mechanizmy `Result` w polaczeniu z typami wyluskowanymi `std::optional`. Uzywane jest RAII przy zapobieganiu wyciekom stanu (czyste usuniecia `clear`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
- `CPythonSkill`: (Singleton) Laduje i serwuje bazowe dane skillow zdefiniowane w plikach txt. (Glowny watek)
- `CPythonSkill::SSkillData`: Struktura z pelna tablica atrybutow jednego skilla (zawiera wlasne wyliczenia, wektory z wymaganiami, wzory w tekstach do klasy `CPoly`).
- `Client::Gameplay::SkillDomain`: Menedzer stanu i czasow odnawiania umiejetnosci. Zarzadza dynamiczna czescia (wymagania uzycia, przeliczanie wg formuly dla danego poziomu umiejetnosci).

**Tabela Metod Publicznych:**
* `CPythonSkill`:
  - `bool RegisterSkillTable(const char * c_szFileName)`: Laduje glowne definicje z tablicy vnum.
  - `bool RegisterSkillDesc(const char * c_szFileName)`: Uzupelnia skille z tablicy o opisy i lokalizacje ikon.
  - `BOOL GetSkillData(DWORD dwSkillIndex, TSkillData ** ppSkillData)`: Pobiera wskaznik na metadane.

* `Client::Gameplay::SkillDomain`:
  - `void DefineSkill(SkillId id, const SkillData& data)`: Wpisuje statyczne wlasciwosci w zasobach domeny.
  - `Result<void, SkillError> CanCast(SkillId skillId, uint32_t currentSP) const`: Zwraca rezultat pomyslny, badz wylicza odpowiedni Error Code z informacja dlaczego skill rzucony byc nie moze.
  - `Result<void, SkillError> UseSkill(SkillId skillId, uint32_t currentSP, uint32_t cooldownMs)`: Odznacza uzycie oraz narzuca nowy cooldown.
  - `void UpdateCooldowns()`: Synchronizuje timery w petli tick.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
Brak specyficznego wyrownania narzuconego `#pragma pack`. Warto zanotowac, ze struktury domeny wykorzystuja wspolczesne typy (`uint32_t`, `std::chrono::milliseconds`). Modul `PythonSkill` trzyma obiekty graficzne pod czystymi pointerami: np. `CGraphicImage * pImage` w `TGradeData`.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)

**Pakiety Sieciowe:**
- Te moduly NIE definiuja wlasnych opcodow ani struktur pakietow sieciowych (CG/GC). 
- Protokol opiera sie o globalne handlery obslugujace akcje rzucenia umiejetnosci, ktore potem wywoluja moduly domenowe by zaktualizowac cooldown po stronie serwera/klienta.

**Metody Pythona (PyMethodDef `s_methods`):**
Modul eksportuje szeroka pule funkcji pod API Python m.in.:
- `SetPathName`, `RegisterSkill`, `LoadSkillData`, `ClearSkillData`
- `GetSkillName`, `GetSkillDescription`, `GetSkillType`, `GetSkillConditionDescriptionCount`, `GetSkillConditionDescription`
- `GetSkillAffectDescriptionCount`, `GetSkillAffectDescription`
- `GetSkillCoolTime`, `GetSkillNeedSP`, `GetSkillContinuationSP`
- `GetSkillMaxLevel`, `GetSkillLevelUpPoint`, `GetSkillLevelLimit`
- `IsSkillRequirement`, `GetSkillRequirementData`, `GetSkillRequireStatCount`, `GetSkillRequireStatData`
- `CanLevelUpSkill`, `IsLevelUpSkill`, `CheckRequirementSueccess`, `GetNeedCharacterLevel`
- `IsToggleSkill`, `IsUseHPSkill`, `IsStandingSkill`, `CanUseSkill`
- `GetIconName`, `GetIconImage`, `GetIconImageNew`, `GetIconInstance`, `GetIconInstanceNew`, `DeleteIconInstance`, `GetGradeData`
- `GetNewAffectDataCount`, `GetNewAffectData`, `GetDuration`, `TEST`

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)

- **Zasady wielowatkowosci:** Obiekty graficzne w `CPythonSkill` jak alokacja CGraphicImage musza odbywac sie w watku glownym renderowania z powodu powiazan z API DirectX. Instancja `SkillDomain` uzywa modyfikatora `mutable std::shared_mutex m_mutex;` do chronienia map cooldownow i stanow (np. modyfikacje SP vs czytanie stanu z innego watku sieciowego pakietow odswiezania) - to wazne by zawsze zachowywac porzadek blokowania.
- **Potencjalne punkty awarii:** Skrypty w Pythonie, czesto po wywolaniu GetIconInstance, musza byc skrupulatne z dealokacja (DeleteIconInstance) poprzez Python, by uniemozliwic memory leaki pointerow VRAM.
- **Zarzadzanie zasobami (RAII):** `SkillDomain` dziala czysto wedlug nowoczesnych standardow (C++20/C++23), korzystajac ze zwalniania w inteligentny sposob - `CPythonSkill` jest wezlem legacy gdzie trzeba recznie uwazac na niszczenie starych danych poprzez `Destroy()`.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")

- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
  Jesli potrzebujesz rozszerzyc opis skillow np. dodanie zupelnie nowej wlasciwosci: 1. Zdefiniuj odpowiedni Enum / Token w `CPythonSkill::ESkillTableTokenType` lub `ESkillDescTokenType`. 2. Dodaj propercje do `SSkillData`. 3. Pobierz te wartosc i umiesc w logice parsera wywolan w Pythonie w pliku `PythonSkill.cpp`. 4. Przekaz wartosc w logice domenowej `SkillDomain` (rozszerz strukture `SkillData` oraz dodaj test logiczny w `CanCast`).
- **Jak debugowac i logowac:** Nalezy korzystac z `EterBase::ModernLogger` (np. LogLevel `Info`, `Error`) i omijac bezposrednie modyfikacje plikow UI z uzyciem std::cout czy `Tracef`. Dla domen uzywaj zwracanych obiektow `Result<void, SkillError>` z biblioteki domenowej by poznawac przyczyne zlych rzadow skilli bez kraszy.
- **Jak testowac bez interfejsu graficznego:** Obiekt `SkillDomain` ma odseparowana architekrure i umozliwia pelne uzycie i stymulowanie jednostkowych testow zastepujac (mockujac) zrodlo serwera poprzez wywolywanie `DefineSkill`, potem symulowanie cooldownu `StartCooldown` i badanie czasu w ticku uzywajac wlasnych fake time pointow. Modul `CPythonSkill` dziala gorzej i bedzie wciagal d3d9 jesli zaladujesz ikony, stad trzeba testowac w trybie opartym o wlasne zasoby headless.

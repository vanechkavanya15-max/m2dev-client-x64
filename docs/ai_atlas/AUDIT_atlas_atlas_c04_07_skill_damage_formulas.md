---
task_id: "atlas_c04_07_skill_damage_formulas"
cluster: "CBT"
module_name: "Kalkulator Formul Obrazen Umiejetnosci"
target_files:
- src/GameLib/SkillDamageFormulaCalculator.h
- src/GameLib/DamageTypes.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c04_07_skill_damage_formulas.md"
architecture_layer: "System Walki, Umiejetnosci i Fizyka Pociskow"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Ten modul stanowi scisle matematyczne jadro kalkulacji obrazen dla umiejetnosci (skilli) wszystkich 4 klas w grze (Wojownik, Ninja, Sura, Szaman). Kod korzysta ze standardu C++23 i operuje czysto w domenie danych, odseparowany calkowicie od warstwy wizualnej, UI oraz grafiki (DirectX/Granny). Obliczenia opieraja sie na statystykach atakujacego (STR, DEX, INT, CON, base attack, magic attack) oraz statystykach obroncy (defense, magic defense, resistance) i zwracaja ostateczna wartosc obrazen.

*   **Przeplyw danych (Data Flow):** Kalkulator przyjmuje dane typu `AttackerStats` i `DefenderStats` wyciagniete z obiektow w trakcie trwania walki. Odbywa sie walidacja i jesli dane sa niepelne, system zwraca wyjatek postaci `std::expected` ze stosownym bledem. Po przejsciu logiki odpowiedniej klasy aplikowane sa redukcje obronne, po czym zwracany jest wynik typu `uint32_t`.
*   **Cykl zycia (Lifecycle):** Kalkulator jest bezstanowy (pure static class). Funkcje sa oznaczone jako `constexpr` i `noexcept`. Wszystkie zmienne sa tymczasowe, alokowane na stosie i uzywane w obrebie pojedynczego wywolania procedury matematycznej. Kod nie posiada stanow trwalych, wiec reset obiektow nie istnieje.
*   **Wywolanie:** Jest ono inicjowane prawdopodobnie wewnatrz instancji systemu walki w trakcie petli update eventu uderzenia z sieci badz symulacji (kiedy nadchodzi pakiet ataku).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
*   **Zaleznosci wejsciowe (Inbound):**
    *   System logiki walki wywolujacy API po otrzymaniu eventu ataku (np. podsystem bazujacy na `InstanceCombatEngine` lub handlery pakietow sieciowych walki w zaleznosci od implementacji serwerowej).
*   **Zaleznosci wyjsciowe (Outbound):**
    *   `EterBase::StrongTypes` - bezpieczny typ ID (np. `EntityId`, `SkillId`, `PlayerLevel`).
    *   Zero powiazan z DirectX czy Pythonem. Modul nie zalezy od wewnetrznego silnika.
*   **Drzewo dyrektyw `#include`:**
    *   `<cstdint>`, `<expected>`, `<optional>`, `<format>`, `<algorithm>`, `<string_view>`, `<stdexcept>`
    *   `"../EterBase/StrongTypes.h"`
    *   Nie istnieje tu ryzyko zaleznosci cyklicznych, to samo dno stosu architektonicznego.
*   **Model pamieciowy:** W tym module korzysta sie wylacznie z bezpiecznych rozwiazan - `std::optional`, kopiowanie/przekazywanie przez stale referencje `const T&`. Nie uzywa sie czystych wskaznikow C. 

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

#### Tabela Klas i Struktur
| Nazwa | Rola | Wlasciciel Watku |
|---|---|---|
| `CombatMath::CalculationError` (enum class) | Kodowanie bledu uzywane w std::expected | N/A (stack) |
| `CombatMath::AttackerStats` (struct) | Pojemnik parametrow atakujacego (id, lvl, str, dex, int, vit, pAtk, mAtk) | N/A (stack) |
| `CombatMath::DefenderStats` (struct) | Pojemnik parametrow obroncy (id, lvl, def, mDef, skillRes) | N/A (stack) |
| `CombatMath::SkillDamageFormulaCalculator` | Agregator metod statycznych wyliczajacych dmg wg klas | Dowolny watek |
| `game_lib::DamageType` (enum class) | Typologia zjawisk uszkadzajacych (Normal, Critical, Penetrate, Poison) | N/A (stack) |

#### Tabela Metod Publicznych
**Dla klasy `SkillDamageFormulaCalculator`**
| Sygnatura | Wartosc zwracana | Pre-conditions | Skutki uboczne |
|---|---|---|---|
| `CalculateWarriorSkillDamage(SkillId, optional<AttackerStats>&, optional<DefenderStats>&, uint8_t)` | `std::expected<uint32_t, CalculationError>` | Attacker, Defender obecne, SkillId != 0 | Brak |
| `CalculateNinjaSkillDamage(...)` | `std::expected<uint32_t, CalculationError>` | Attacker, Defender obecne, SkillId != 0 | Brak |
| `CalculateSuraSkillDamage(...)` | `std::expected<uint32_t, CalculationError>` | Attacker, Defender obecne, SkillId != 0 | Brak |
| `CalculateShamanSkillDamage(...)` | `std::expected<uint32_t, CalculationError>` | Attacker, Defender obecne, SkillId != 0 | Brak |

**Dla namespace `game_lib`**
| Sygnatura | Wartosc zwracana | Pre-conditions | Skutki uboczne |
|---|---|---|---|
| `constexpr std::string_view ToString(DamageType)` | `std::string_view` | Typ `DamageType` musi byc zdefiniowany | Rzuca `std::invalid_argument` dla nieznanego enuma |

#### Pamieciowy Layout Struktur (Memory Layout & Offsets)
Poniewaz sa to proste typy POD uzywajace C++23 std::expected / optional, rozmiar zalezy od ABI i paddingu EterBase::StrongType, jednakze brak tam pol pointerowych do virtual methods table (vtable), wiec calosc sklada sie z kolejnych zmiennych liczbowych typu `uint32_t` i `uint8_t`.
*   `AttackerStats`: Zawiera id, level oraz 6 elementow uint32_t (statystyki i atak). Rozmiar okolo 32-40 bajtow w zaleznosci od implementacji StrongType.
*   `DefenderStats`: id, level, defense (uint32), magicDefense (uint32), resistance (uint8). 

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
*   **Pakiety Sieciowe:** Powiazane bezposrednio (logicznie) z wymiana pakietow atakow (np. `HEADER_CG_ATTACK`, `HEADER_GC_DAMAGE_INFO`). Ten konkretny naglowek w zadnym stopniu nie zajmuje sie parsowaniem pakietow - otrzymuje surowe dane C++.
*   **Python C-API:** Brak bezposredniego mapowania via `PyMethodDef`. Wynik kalkulacji moze byc przekazywany wyzej jako argument wezwania logujacego dmg UI (render float text), ale klasa sama tego nie eksportuje do interfejsu uzytkownika Pythona.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
*   **Zasady wielowatkowosci:** Funkcje klasy `SkillDamageFormulaCalculator` sa bezpieczne watkowo (Thread-Safe), poniewaz nie korzystaja ze stanu globalnego i dzialaja na tymczasowych kopiach argumentow umieszczonych na stosie. Mozna je rownolegle wykonywac na wielu watkach roboczych (np. Worker Threads).
*   **Punkty awarii (Crash Points & Edge Cases):**
    *   Blad brakujacych struktur - zwroci `CalculationError::AttackerStatsMissing` lub `DefenderStatsMissing`.
    *   Minimalne obrazenia to 1. Zostalo to poprawnie zabezpieczone logicznie (`std::max(1.0f, finalDamage)`).
    *   Ryzyko wyjatku w `ToString(DamageType)` - rzuci zdefiniowany w systemie wyjatek `std::invalid_argument` w przypadku niewalaskiego castingu enuma przez inne czesci systemu (wazne dla AI analizujacego logi).
*   **RAII / Zasoby:** Kod ten nie zajmuje na stale zasobow CPU ani RAMu. Posiada obrys pamieci O(1).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
*   **Dodawanie nowej funkcji:**
    1.  Otworz `src/GameLib/SkillDamageFormulaCalculator.h`.
    2.  Zadeklaruj w klasie funkcje statyczna `CalculateNewClassSkillDamage` wedlug istniejacych sygnatur.
    3.  Zaimplementuj ponizej w namespace nowa logike uzywajac typow wejsciowych `AttackerStats` oraz `DefenderStats`.
    4.  Przetestuj dodana logike piszac nowy unit test porownujacy z przewidywana stala wartoscia bazowa obrazen.
*   **Jak debugowac i logowac:** Wyniki poszczegolnych faz mozemy logowac, np. za pomoca uzywania narzedzi takich jak `EterBase::ModernLogger`, poniewaz modul zwraca wartosci wyrazne i przewidywalne. Brak jest zmiennych stanow do nadzorowania przez wewnetrzne narzedzia. Zwracany typ `std::expected` zawiera informacje o statusie bez przerywania dzialania calego algorytmu.
*   **Jak testowac (Headless / Unit Test Harness):** Poniewaz kod jest odizolowany, testowanie moze byc przeprowadzone latwo w odizolowanym srodowisku C++23. Stworz narzedzie doctest inicjujace mockowe obiekty `AttackerStats` i `DefenderStats` oraz zrownaj z recznie obliczonym standardem, z uwagi na uzycie zmiennoprzecinkowych obliczen float uzyj zblizeniowego sprawdzania wartosci. Nie potrzebujesz wirtualnej instancji okna by testowac ten system.

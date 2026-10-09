---
task_id: "atlas_c07_10_particle_system"
cluster: "MOD"
module_name: "CParticleSystem - Fizyka Czasteczek Efektu"
target_files:
- src/EffectLib/ParticleSystem.cpp
- src/EffectLib/ParticleSystemData.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_10_particle_system.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja w architekturze**: Ten modul stanowi fundament systemu wizualnego dla efektow specjalnych (np. skille, uderzenia, aury) opartych na czasteczkach w grze. Odpowiada za definiowanie parametrow czasteczek, takich jak ich wlasciwosci (CParticleProperty) i emitery (CEmitterProperty), oraz za instancjonowanie ich podczas rozgrywki za pomoca CParticleSystemInstance. Klasa CParticleSystemData z kolei definuje model danych odczytywalny ze skryptow (np. TextFileLoader). 
- **Wywolanie w petli gry**: Logika systemu czasteczek wywolywana jest podczas renderowania i aktualizacji stanu efektow. Aktualizacja stanu i powstawanie nowych czasteczek maja miejsce podczas `OnUpdate` (czyli w petli logiki efektu), a wizualizacja zachodzi w czasie fazy `OnRender` w petli graficznej (zazwyczaj wywolywane z EffectManager).
- **Control Flow & Data Flow**: 
  1. Skrypty efektow (w formacie tekstowym) sa parsowane przez `CParticleSystemData::OnLoadScript`, co inicjalizuje wlasciwosci emiterow oraz czasteczek.
  2. Kiedy efekt ma byc wyswietlony, powstaje instancja `CParticleSystemInstance`. 
  3. Wywolanie metody `CreateParticles` podczas `OnUpdate` alokuje nowe struktury `CParticleInstance` wykorzystujac zdefiniowany wczesniej ksztalt (punkt, elipsa, prostokat, sfera) i wylicza odpowiednia predkosc oraz pozycje startowa. 
  4. Nastepnie poszczegolne czasteczki zyja wewnatrz tablicy list `m_ParticleInstanceListVector`, rozdzielone wedlug indeksow ramek, sa one aktualizowane przez wlasna metode `Update` i renderowane przy pomocy struktur zdefiniowanych w `NParticleRenderer` podczas `OnRender`.
- **Cykl zycia**: W systemie uzyto `CDynamicPool` dla zarzadzania instancjami (zarowno dla `CParticleSystemData` jak i `CParticleSystemInstance`), co optymalizuje alokacje. Inicjalizacja instancji wywoluje `OnInitialize`, po czym podczas zniszczenia `OnDestroy` wszystkie czasteczki z instancji sa deallokowane (za pomoca wlasnej deallokacji do puli `DeleteThis`), podobnie jak powiazane teksturowe obiekty wizualne (`CGraphicImageInstance`).

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound)**: 
  - `CEffectData` (uzywa `CParticleSystemData`) 
  - `CEffectInstance` (zarzadza zbiorem wektorow `CParticleSystemInstance`)
  - `CEffectManager` (pule pamieci sa logowane)
  - Pliki tekstowe zasobow ladowane przez `CTextFileLoader`.
- **Zaleznosci wyjsciowe (Outbound)**: 
  - `CEffectElementBase`, `CEffectElementBaseInstance`
  - `CParticleInstance`, `CParticleProperty`, `CEmitterProperty`
  - Direct3D 9 (`D3DXMATRIX`, `STATEMANAGER`, operacje matematyczne D3DX)
  - `EterLib`: `CGraphicImageInstance`, `StateManager`, `GrpScreen`, algorytmy frustum culling.
  - Generatory liczb losowych z `EterBase/Random.h`
- **Drzewo dyrektyw `#include`**: 
  - `ParticleSystemData.h` zawiera `<EterLib/TextFileLoader.h>`, `"EffectElementBase.h"`, `"EmitterProperty.h"`, `"ParticleProperty.h"`. W `ParticleSystemData.cpp` dochodza `"StdAfx.h"` i `"ParticleInstance.h"`.
  - `ParticleSystemInstance.h` zalezy od `"EffectElementBaseInstance.h"`, `"ParticleInstance.h"`, `"ParticleProperty.h"`, `<Eterlib/GrpScreen.h>`, `<Eterlib/StateManager.h>`, `<EterLib/GrpImageInstance.h>`, `"EmitterProperty.h"`. W `ParticleSystemInstance.cpp` dodatkowo dolaczono `<EterBase/Random.h>`.
- **Model pamieciowy**:
  - `CParticleSystemData` uzywa twardych alokacji pamieciowych poprzez wlasny system pul objetych w `CDynamicPool`. Zwracane sa czyste wskazniki `CParticleSystemData *`.
  - `CParticleSystemInstance` posiada zagniezdzone pule, z ktorych rozdziela obiekty. W obrebie klasy znajduja sie surowe wskazniki w kontenerach (`std::list<CParticleInstance*>` i `std::vector<CGraphicImageInstance*>`).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur**
- `CParticleSystemData`
  - Rola: Zbior wlasciwosci (ParticleProperty, EmitterProperty) parsowany z pliku, sluzy jako szablon (wzorzec). Dziedziczy z `CEffectElementBase`.
  - Wielkosc: Zalezy od klas wbudowanych takich jak `CEmitterProperty` i `CParticleProperty`. Posiada pule instancji `ms_kPool`.
- `CParticleSystemInstance`
  - Rola: Konkretna instancja efektu emitora na scenie w okreslonym czasie. Dziedziczy z `CEffectElementBaseInstance`. Zarzadza cyklem zycia wyemitowanych jednostek (`CParticleInstance`).
  - Pola: `m_dwCurrentEmissionCount`, listy czasteczek (`m_ParticleInstanceListVector`), zaleznosci od wzorcow.

**Tabela Metod Publicznych (`CParticleSystemData`)**
- `CParticleSystemData* New()`: Pobiera z puli, zwraca wskaznik. Skutek: rezerwuje zasob.
- `void Delete(CParticleSystemData* pkData)`: Czysci zasoby wywolujac `Clear()` i zwraca do puli `ms_kPool`.
- `CEmitterProperty* GetEmitterPropertyPointer() / CParticleProperty* GetParticlePropertyPointer()`: Zwraca surowe referencje do zasobow z wnetrza klasy. Warunek wejsciowy: klasa musi byc poprawnie skonstruowana.
- `void ChangeTexture(const char* c_szFileName)`: Podmienia tekstury animowane. Wymaga poprawnego pliku (str).

**Tabela Metod Publicznych (`CParticleSystemInstance`)**
- `CParticleSystemInstance* New()`: Alloc z dynamicznej puli.
- `void Delete(CParticleSystemInstance* pkData)`: Niszczy referencje w srodku poprzez wywolanie Destroy(), czysci z puli.
- `void OnSetDataPointer(CEffectElementBase * pElement)`: Inicjalizuje wskazniki do danych elementow bazowych systemu. Rezerwuje odpowiednie kolekcje na animacje w zaleznosci od specyfikacji. 
- `DWORD GetEmissionCount()`: Zwraca obecna liczbe emitowanych czasteczek. Bez side effects.
- `void CreateParticles(float fElapsedTime)`: Wylicza, alokuje i wklada na odpowiednie listy nowe `CParticleInstance` na podstawie odstepu czasowego i wlasciwosci emitera. Zmienia stan kontenera `m_ParticleInstanceListVector`.

**Pamieciowy Layout Struktur (Memory Layout & Offsets)**
- Obiekty powiazane z CParticleSystemData uzywaja wektorow STD dla stanow klatek co implikuje brak rygorystycznej definicji offsetu. Wszelkie wlasciwosci powinny byc czytane przez gettery lub interfejs eventow, a ich modyfikacja musi odbywac sie z najwyzsza ostroznoscia poprzez wskazniki klasowe bazujac na `mc_pmatLocal` w instancji dla offsetow przestrzeni.
- Renderery ukryte (np. `NParticleRenderer::TwoSideRenderer`) polegaja na strukturze `TPTVertex` zdefiniowanej w innym miejscu, wykorzystywanej przez wbudowane metody DrawPrimitiveUP DX9.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe**: Ten modul nie wchodzi w bezposrednia interakcje z pakietami sieciowymi. Jest wywolywany lokalnie w momencie, kiedy pakiety serwerowe lub eventy gry kaza klientowi odegrac jakas animacje badz uderzenie (zazwyczaj poprzez menadzery takie jak CEffectManager operujace na bazowych interfejsach).
- **Metody Pythona**: Sam obiekt nie posiada dedykowanego modulu w Pythonie. Konfiguracja ladowana jest natywnie przez narzedzie `TextFileLoader` z formatu wykorzystywanego wewnetrznie przez klienta. Brak API C++ <-> Python dla samej instancji emiterow. 

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci**: Poniewaz obiekt bezposrednio dotyka biblioteki Direct3D 9 z wnetrza funkcji `OnRender` (poprzez `STATEMANAGER`), to wszystkie metody uzywajace API DirectX i manipulujace rendererami MUSZA byc wykonywane jedynie na glownym watku wywolujacym D3D9. Inaczej grozi to `D3DERR_INVALIDCALL`.
- **Pulapki (Crash Points & Edge Cases)**:
  - Pulapki w zarzadzaniu pamiecia: Zla operacja usuwania dla `CParticleInstance` ze wnetrza CParticleSystemInstance (np. `Update` nie zwraca poprawnej wartosci bool - false przy smierci) moze sprawic, iz zwrocony pointer do `ms_kPool` zostanie zasyfiony nowymi deallokacjami, skutkujac zepsuta pula dynamiczna (Use-After-Free).
  - W `CParticleSystemInstance::CreateParticles` wartosc fLifeTime na podstawie float point zero uzyta do zwrocenia przed czasem - bardzo cienka granica bez epsilon. Utrata stabilnosci jesli wartosci wyliczone ze skryptu tekstowego beda wadliwe badz ujemne dla wielkosci pamieci na frame'y (`m_ParticleInstanceListVector.resize()`).
- **Zarzadzanie zasobami (RAII)**: Architektura polega na dedykowanym wzorcu tworzenia poprzez pule wbudowane (Object Pooling) via `ms_kPool`. Destrukcja manualna wymaga wywolania Delete z puli zamiennie - inteligentne pointery std:: nie sa uzyte, stad duze ryzyko wycieku wlasciwosci w przypadku wystapienia wewnetrznego bledu `std::bad_alloc` we wbudowanych kolekcjach STL.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji**:
  1. Zaprojektuj nowa wlasciwosc w klasie `CEmitterProperty` badz `CParticleProperty`. 
  2. Dodaj pobieranie wlasciwosci z pliku w `CParticleSystemData::OnLoadScript`, wykorzystujac metody CTextFileLoader.
  3. Wykorzystaj ta nowa wlasciwosc w `CParticleSystemInstance::CreateParticles` do nadpisania pozycjonowania/rotacji bazowej generowanego `CParticleInstance`.
- **Debugowanie**:
  - Breakpoint w okolicach `CreateParticles` to podstawa aby podsluchiwac, co powoduje nienaturalnie krotki badz zbyt duzy opad czasteczek na mape.
  - Nalezy rejestrowac calkowita ilosc rezerwacji z polecenia ms_kPool.GetCapacity(), jak uzyte to jest w `CEffectManager`, co pozwala sledzic wycieki. 
- **Testowanie**: Poniewaz uzywana zaleznosc D3DX, nalezy utworzyc w testach srodowisko testujace przy uzyciu makra "dummy_include" w celu izolacji naglowkow API DX9 (udawana inicjalizacja `TPTVertex` i `D3DXVECTOR`), co umozliwi mockowanie transformacji pozycji wektorow i logiki Update bedacej agnostykiem renderowania w narzedziu uzywajacym g++.  Poniewaz to modul wylacznie logiczny na wektorach, odpalenie na nim `run_isolated_test.sh` wykazaloby, ze matematyka odliczen petli dziala.

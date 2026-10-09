---
task_id: "atlas_c07_09_effect_instance"
cluster: "MOD"
module_name: "CEffectInstance - Instancja Aktywnego Efektu Czasteczkowego"
target_files:
- src/EffectLib/EffectInstance.cpp
- src/EffectLib/EffectInstance.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_09_effect_instance.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul ten odpowiada za zarzadzanie aktywnymi instancjami efektow wizualnych i dzwiekowych w grze (czasteczki, siatki, swiatla). CEffectInstance pelni role kontenera dla roznych elementow efektu zdefiniowanych w CEffectData, takich jak systemy czasteczkowe (ParticleSystemInstance), animowane siatki 3D (EffectMeshInstance) i zrodla swiatla (LightInstance). Klasa zarzadza cyklem zycia efektu, od alokacji pamieci przez CDynamicPool, poprzez inicjalizacje i przypisanie danych z CEffectData, po proces aktualizacji (OnUpdate) transformacji globalnych i odtwarzanie dzwiekow, az do wyrenderowania efektu za pomoca DirectX (OnRender). Kod w OnUpdate jest wywolywany w glownej petli gry na etapach aktualizacji logiki i animacji obiektow 3D, a w OnRender podczas fazy rysowania.

Przeplyw danych i cykl zycia obiektow opiera sie na uzyciu CDynamicPool dla optymalnego zarzadzania instancjami i ponownego ich uzycia, minimalizujac fragmentacje pamieci i koszt alokacji. CEffectInstance nie dealokuje recznie pamieci dla wlasnych elementow czasteczek w swoim destruktorze, ale wymaga explicite wywolania metody Delete(), ktora czysci (metoda Clear) mniejsze wektory pod-instancji.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul ten jest wywolywany przez wyzsze struktury zajmujace sie efektami, zazwyczaj z poziomu Menedzera Efektow (CEffectManager / CPythonEffectManager). Renderowanie efektu nastepuje najpewniej z poziomu CGraphicObjectInstance (jako zaleznosc dziedziczenia) oraz menedzera renderingu.
- **Zaleznosci wyjsciowe (Outbound):** Kod bezposrednio wola API DirectX 9 poprzez abstrakcje STATEMANAGER do zmiany stanow renderingu (Alpha Blend, Alpha Test, Cull Mode, itp.). Ponadto korzysta z CTimer (do sledzenia uplywu czasu dla animacji czasteczek) oraz SoundEngine z AudioLib do obslugi odtwarzania dzwiekow przyczepionych do przestrzeni w jakiej dany efekt istnieje. Zarzadzanie pamiecia dla efektow uzywa narzedzia CDynamicPool.
- **Drzewo dyrektyw `#include`:** Wewnetrzne zaleznosci: EffectElementBaseInstance.h, EffectData.h, EffectMeshInstance.h, ParticleSystemInstance.h, SimpleLightInstance.h, Eterlib/GrpObjectInstance.h, Eterlib/Pool.h, AudioLib/Type.h. W pliku implementacyjnym .cpp takze EterBase/Stl.h, EterLib/StateManager.h, AudioLib/SoundEngine.h. Kod ma potencjalne zaleznosci cykliczne wynikajace ze zlozonosci cyklu zycia miedzy abstrakcja efektu a konkretnymi instancjami systemow czasteczkowych, jednak na ten moment operuje on za pomoca dobrze zorganizowanych zbiorow naglowkow.
- **Model pamieciowy:** W klasie powszechnie stosuje sie surowe wskazniki C (*). Pamiec calego obiektu obslugiwana jest poprzez globalna pule ms_kPool (instancja CDynamicPool<CEffectInstance>). Nie wystepuja tu inteligentne wskazniki z biblioteki standardowej (jak std::unique_ptr).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
- `CEffectInstance` - Glowna klasa, dziedziczy z CGraphicObjectInstance, posiada ID rzedu EFFECT_OBJECT. Cykl zycia jest zalezny od statycznej puli `ms_kPool`. Zawiera logiczny podzial zaleznosci renderowania (czasteczki, siatki 3D, swiatlo), wlascicielem jest glowny watek renderujacy DirectX. Posiada obiekty D3DXVECTOR3 oraz D3DXMATRIX. W zwiazku z alokacja pool, sizeof jest staly dla instancji w puli i operuje na zdefiniowanych std::vector oraz wskaznikach na systemy dzwieku i dane.

**Tabela Metod Publicznych (`CEffectInstance`):**
- `static CEffectInstance* New();` - Zwraca nowo zaalokowany surowy wskaznik na instancje efektu uzywajac poola alokacji `ms_kPool`.
- `static void Delete(CEffectInstance* pkEftInst);` - Czysci wszystkie zaleznosci powiazane z `pkEftInst` a nastepnie zwalnia zajmowana przez niego pamiec i zwraca do poola pamieci.
- `void Clear();` - Metoda destrukcyjna usuwajaca systemy czasteczek, meshes i swiatla ze wewnetrznych tablic vector.
- `void SetEffectDataPointer(CEffectData * pEffectData);` - Przypisuje instancji metadane efektu, inicjalizuje wszystkie sub-instancje czasteczek, meshes i swiatla oraz przypisuje parametry sferyczne kolizji. Rejestruje takze zrodla dzwieku.
- `void OnUpdate();` - Wewnetrzna funkcja updatu zintegrowana ze struktura FEffectUpdator i CTimer do wewnetrznej petli gry; uaktualnia uplyw czasu na kazdym podpietym komponencie czasteczek, siatki, swiatla. W przypadku martwego efektu ustawia m_isAlive na FALSE.
- `void OnRender();` - Centralny punkt wejsciowy renderowania efektu bezposrednio do DirectX API przez STATEMANAGER z modyfikacja renderowania (np. AlphaBlend, SrcBlend) bez zapisu do osi Z. Uzywa FVF D3DFVF_XYZ | D3DFVF_TEX1.
- `void UpdateSound();` - Odtwarza lub uaktualnia podpiete dzwieki 3D na podstawie D3DXMATRIX korzystajac z SoundEngine z AudioLib.

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- Wskazniki surowe na struktury pochodne od CEffectElementBaseInstance zgromadzone sa w dwoch glownych wektorach: `m_ParticleInstanceVector`, `m_MeshInstanceVector` oraz zrodla swiatla `m_LightInstanceVector`.
- Wskaznik na baze dzwiekow `m_pSoundInstanceVector`.
- Dane z transformacji przechowywane sa m.in. w macierzy globalnej `m_matGlobal` z biblioteki DirectX 9 oraz polach `m_fBoundingSphereRadius`, `m_v3BoundingSpherePosition` obslugujacych obwiednie (culling, kolizje sfer).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- Ten plik implementuje calkowicie kliencka strukture dla silnika 3D / DirectX.
- **Pakiety Sieciowe:** Kod nie wykonuje i nie obsluguje zadnych zadan z powiazaniami pakietow sieciowych serwera (brak wystapien TPacket). Ewentualne wymuszanie efektow pakietem nastepuje gdzies w kodzie instancji charakterow, a nie tutaj.
- **Metody Pythona:** Ten konkretny interfejs nie operuje bezposrednio na C-API Pythona (brak PyMethodDef). Zewnetrzne systemy Pythona (takie jak CPythonEffectManager/chrmgr) manipuluja ta klasa zewnetrznie i posrednio operuja na alokatorach tu utworzonych.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystkie dzialania musza byc wykonywane glownie i wylacznie w glownym watku, powiazanym z cyklem D3D (szczegolnie wywolania OnRender uzywajace STATEMANAGER i operujace na surowych wektorach STL).
- **Zarzadzanie zasobami (RAII) & Nullowe wskazniki:** Zniszczenie obiektu poza standardowym procesem usuwania (np. brak wezwania CEffectInstance::Delete i poleganie wylacznie na operatorze `delete`) NIE WYWOLA operacji czyszczenia zawartosci w `Clear()`. Metoda `~CEffectInstance` wykonuje na koncu jedynie operacje `assert` na pustosc tablic. Kod rzuca bledy asercji, co oznacza ze agent powinien zawsze uzywac `CEffectInstance::Delete`. Metoda `GetTextTrack()` moze operowac na nieokreslonych obszarach jesli ktos nie odwolal sie od bezpiecznych funkcji jak w CGrannyMotion, wiec tu rowniez nalezy wziac uwage ze CEffectInstance moze posiadac wektory z pustymi surowymi wskaznikami na CEffectElementBaseInstance, jesli cos w CEffectData zostalo zle zdefiniowane. Brak jest dodatkowej weryfikacji nullptr w glownych algorytmach `OnUpdate` (uzywa on std::for_each i zaklada idealnie dzialajace pamieci instancji elementow).
- Zaleznosc macierzy dzwieku (SoundEngine) wydobywana bezposrednio z `m_matGlobal._41`, `_42`, `_43` co narzuca ze podsystem swiata 3D i dzwiek uzywa tej samej transformacji osiowej bez zadnych offsetow rotacyjnych.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** Wszelkie dodawane nowe typy efektow nalezy wpierw wprowadzic w CEffectData jako osobny obiekt danych i zarejestrowac go w petlach tablic z SetEffectDataPointer (np. dla nowego typu efektu wibracji, zdefiniuj metode `__SetVibrationData` dodajac go do nowego vectora `m_VibrationInstanceVector`). Nastepnie odpowiednio go odswiezyc w funkcji `OnUpdate` iteratorem/wyrazeniem FEffectUpdator i wyrenderowac w `OnRender`. Nie zapomnij dodac funkcji uwalniania z wewnetrznej puli efektu wewnatrz glownej petli wyczyszczenia `Clear()`.
- **Jak debugowac i logowac:** Nalezy dodac break-pointy w `OnUpdate`, `SetEffectDataPointer` badajace wartosci transformacji `m_matGlobal` lub stanu w jakim dany watek operuje. Poniewaz instancje sa cesto recyklingowane przez globalnego poola `ms_kPool`, wazne jest zbadanie wartosci przed wejsciem do puli, i sprawdzenie w `__Initialize` podczas recyklingu zmiennych obiektu czy stary stan nie "przecieka".
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** Modul opiera sie w wiekszosci na obslugach DX9 przez `STATEMANAGER`. Testowanie na sucho bedzie wymagalo za-mockowania calosci przestrzeni `STATEMANAGER`'a (mockujace D3D, D3DTSS_, itp.) wedlug globalnej wskazowki MockStateManager. Oprocz tego nalezy mockowac system alokacji `CDynamicPool`. Zalezne elementy testu moga takze wymagac CTimer (jako mocka umozliwiajacego symulacje petli gry i `GetCurrentSecond`).

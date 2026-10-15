---
task_id: "atlas_c07_11_effect_mesh_anim"
cluster: "MOD"
module_name: "CEffectMesh - Dynamiczne Siatki Efektowe i Rozblyski"
target_files:
- src/EffectLib/EffectMesh.cpp
- src/EffectLib/SimpleLightInstance.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_11_effect_mesh_anim.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul EffectLib, a w szczegolnosci pliki `EffectMesh.cpp` oraz `SimpleLightInstance.cpp`, odpowiada za wizualna i kinetyczna strone "magii" w swiecie gry. Kod ten implementuje mozliwosc ladowania wlasnych, ruszajacych sie siatek (np. bable wokol leczenia) oraz generowania dynamicznych rozblyskow swiatla dla efektow specjalnych (np. swiatlo rzucane na otoczenie podczas wybuchu ognia). 

- **Funkcja w architekturze:** Modul funkcjonuje jako mechanizm kliencki zarzadzajacy danymi geometrycznymi (z rozszerzeniem np. `.mde`) oraz zrodlami swiatla `D3DLIGHT9` przypietymi do odtwarzanych animacji (np. rzucania skilli).
- **Punkt wywolania (Petla gry):** Cykl logiczny dla instancji zrodla swiatla zarzadzany jest poprzez metode `OnUpdate` (wywolywana podczas aktualizacji silnika EterLib), gdzie obliczane jest zanikanie swiatla, jego lokalny czas oraz przestrzenne polozenie, nastepnie wysylane do Menedzera Swiatla (`CLightManager`).
- **Przeplyw danych:** Kod bazuje na koncepcie "Skrypt/Zasob -> Instancja". Obiekty typu `CLightData` (ladowane przez `CTextFileLoader`) oraz `CEffectMeshScript` stanowia niezmienne zrodlo prawdy dla wlasciwosci (kolor, skala, promien swiatla, tablice czasowe), ktore nastepnie kopiowane sa do powolanych instancji typu `CLightInstance`. W trakcie gry `CLightInstance` wola funkcje D3D poprzez `CLightManager`.
- **Cykl zycia (Lifecycle):** Alokacja odbywa sie bez uzycia domyslnego sterty systemowej (`new/delete`), a zamiast tego uzywana jest pula manualnych, przerezerwowanych blokow pamieci klasowych `CDynamicPool<T>`. Nalezy pobrac obiekt przez `New()` i zawsze oddac przez `Delete()`. W czasie zakonczenia aplikacji wywolywane sa metody `DestroySystem()`.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Modul powolywany jest do zycia przez glowne klasy `CEffectManager`, podczas odgrywania efektow `CEffectInstance` na postaciach (`CInstanceBase`) badz otoczeniu. 
- **Zaleznosci wyjsciowe (Outbound):** Kod bezposrednio oddzialuje na warstwy `EterLib` (`GrpLightManager.h`, `GrpScreen.h`, `TextFileLoader.h`, `ResourceManager.h`), podsystem ladowania plikow `PackLib` (Virtual File System) oraz biblioteki bazowe DirectX 9 (D3DXVECTOR3, D3DLIGHT9, operatory blendowania D3D).
- **Drzewo dyrektyw `#include`:** Zaleznosci to w szczegolnosci `<d3dx9.h>`, `Type.h`, `EffectElementBase.h`, `SimpleLightData.h`. Nalezy uwazac na inkluzje krzyzowe poprzez klasy bazowe `EffectElementBaseInstance.h`.
- **Model pamieciowy:** Dominuja tu "czyste" wskazniki C (`CLightInstance*`, `CLightData*`, `CEffectMesh::SEffectMeshData*`) bez owijania w RAII (`std::unique_ptr`). Kolekcje takie jak wektory siatek uzywaja `std::vector` i sa trzymane w pamieci po parsowaniu. 

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):

**Tabela Klas i Struktur:**
- `CEffectMesh` - Klasa dziedziczaca po `CResource`. Zarzadza binarnym modelem (.mde). Odpowiada za wczytanie danych klatek (FrameData), wierzcholkow i map UV do buforow uzywajac metody `OnLoad`. Wlasciciel: watek ladowania lub glowny. 
- `CEffectMesh::SEffectMeshData` - Wewnetrzna struktura przetrzymujaca wektor wczytanych tekstur dyfuzyjnych oraz wierzcholkow w czasie animacji. Alokowana przez `ms_kPool`.
- `CEffectMeshScript` - Klasa dziedziczaca po `CEffectElementBase`, parsujaca wlasciwosci siatki 3D, takie jak `billboardtype`, operatory nakladania kolorow (BlendMode) i animacje tekstur.
- `CLightData` - Kontener na wlasciwosci zasobu swietlnego z pliku skryptowego (promien, duration, tlumienie, kolor ambient/diffuse). Utrzymuje tablice zdarzen czasowych (zmian promienia swiatla).
- `CLightInstance` - Klasa realizujaca logiczna i przestrzenna emanacje obiektu. Aktualizuje fizyczne parametry globalnego API w `OnUpdate`. Wlasciciel: watek glowny.

**Tabela Metod Publicznych:**
- `CEffectMesh::OnLoad(int iSize, const void * c_pvBuf)` -> bool. Sygnatura wejsciowa do ladowania binarnego bufora. Posiada obsluge wstecznej kompatybilnosci formatow `EffectData` i `MDEData002`.
- `CLightInstance::OnUpdate(float fElapsedTime)` -> bool. Wylicza biezace tlumienie i zasieg swiatla, a nastepnie transformuje wektory relatywne efektu do wektorow globalnych w przestrzeni uzywajac macierzy `mc_pmatLocal` oraz `D3DXVec3TransformCoord`. Aktualizuje instancje z D3D.
- `CLightInstance::OnSetDataPointer(CEffectElementBase * pElement)` -> void. Ustawia podstawe swiatla (rzutuje zasob ogolny `pElement` na `CLightData`) oraz rejestruje identyfikator w `CLightManager`. 

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
- W obiekcie `CLightInstance` niefortunne ustawienie offsetow powoduje, ze flaga `m_dwRangeIndex` uzywana do starych (obecnie zablokowanych) mechanik zaleznosci czasowych interpolacji swiatla moglaby znajdowac sie tuz za zywym wskaznikiem na obiekt.
- Binarne bufory klatek wczytywane w `CEffectMesh::__LoadData_Ver002` sa serializowane klatka po klatce i trzymane w liniowych wektorach na wezly. AI analizujace layout offsetow powinno podlaczac sie pod struktury klatek, by modyfikowac w locie wektory UV.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Modul calkowicie pozbawiony mostkow sieciowych (izolowany wizualnie).
- **Metody Pythona (`PyMethodDef`):** Brak wyeksponowanych metod API skryptowego dla `CLightInstance` lub `CEffectMesh` (obiekty generowane za rzadaniem `CInstanceBase` oraz `CPythonCharacterManager`).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Kod narzuca scisle reguly watku glownego. `CDynamicPool<T>::ms_kPool` w uzytych klasach nie wspiera operacji atomowych. Alokacja efektu z watku sieciowego spowoduje natychmiastowe uszkodzenie sterty (Heap Corruption).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Brak obslugi pustych wektorow w `CEffectMesh::GetMeshDataPointer` poza prymitywnym `assert(dwMeshIndex < m_pEffectMeshDataVector.size());`. Jesli asercja ulegnie wylaczeniu (kompilacja Release), AI narazi klienta na odczyty z niewlasciwej pamieci. Metoda `CLightInstance::OnDestroy` bazuje na ukrytym id, co uniemozliwia zwrocenie dwoch zrodel tego samego ID przy restarcie efektu.
- **Zarzadzanie zasobami (RAII):** Zasoby powolane funkcjami `New()` nie uzywaja semantyki RAII (unique_ptr). Wycieki sterty pul (Leak/Memory Fragmentation) moga nastapic, jesli instancje w CEffectManager nie wywolaja `Delete()` lub zgubia swoj ID przed oddaniem bufora do Managera Swiatel. W przypadku brakow w konfiguracji VFS, kod cofa operacje bez wyczyszczenia wektorow (szukaj np: wczesnego powrotu z OnLoad_Ver002).

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jesli edytujesz nowa wlasciwosc materialowa efektu, dodaj zmienna w `TMeshData` pliku `EffectMesh.h`. 
  2. Nastepnie dodaj wczytywanie metody na styk (parser get token string/float) wewnatrz `CEffectMeshScript::OnLoadScript`.
  3. Zmodyfikuj instancjonowanie nowej wlasciwosci w warstwie renderujacej. Pamietaj: `EffectMesh` sluzy do parsu a obiekty w EterLib::Renderer interpretuja flagi typu `byBlendingDestType`.
- **Jak debugowac i logowac:** Do monitorowania bledow ladowania zasobow wirtualnych uzywaj klas `LogBox` oraz sprawdzaj flage zwracana przez `CPackManager::Instance().GetFile()`. Wykorzystaj istniejace zmienne logowania, lub dodaj output diagnostyczny w klasie TextFileLoader.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** ABY przetestowac klase instancji swiatla `CLightInstance` (wymagana w testach AI atlasu), zbuduj tzw. "mock system". Zamockuj wywolania `CLightManager::Instance().RegisterLight` poprzez przeciazenie klasy lub `#define` podstawiajacy falszywy manager symulujacy dodanie do wektora, dzieki czemu kod `OnUpdate` odliczy czas petli bez uruchamiania calego stacku Direct3D. Upewnij sie, ze `#include <d3dx9.h>` w tescie nie nadpisze Mock'a i odwrotnie (uzywaj dyrektyw `_D3D9_H_` itp.).

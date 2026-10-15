---
task_id: "atlas_c07_12_cloth_wings_sim"
cluster: "MOD"
module_name: "Symulacja Ruchu Szarf i Skrzydel Kostiumowych"
target_files:
- src/UserInterface/InstanceControllers/InstanceClothWingsController.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c07_12_cloth_wings_sim.md"
architecture_layer: "Modele 3D, Szkielety Granny, Drzewa i Efekty"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Ten modul zarzadza symulacja ruchu (falowania) szarf oraz skrzydel kostiumowych przypisanych do postaci gracza, zsynchronizowana z ruchem postaci i systemem animacji. Zapewnia on takze detekcje kolizji, aby zapobiec przenikaniu materialu przez zbroje (clipping). 

Modul jest wywolywany w glownej petli gry podczas fazy OnUpdate w celu aktualizacji transformacji kosci szarfy w zaleznosci od fizyki (np. opor wiatru, przyspieszenie postaci). Dodatkowo zintegrowany jest z faza OnRender w Granny 3D, by przekazac przeliczone macierze kosci na karte graficzna w celu renderowania (vertex skinning).

Przeplyw danych: 
1. Postac przyspiesza (zmiana wektora predkosci).
2. InstanceClothWingsController przelicza sily na wierzcholki szarfy.
3. System fizyki naklada ograniczniki by szarfa nie przecieta zbroi gracza.
4. Przeliczone macierze transformacji sa aktualizowane i przesylane do potoku renderowania Granny.

Cykl zycia:
- Alokacja: Tworzony wraz z modelem szarfy podpietym do modelu bazowego gracza (CInstanceBase).
- Inicjalizacja: Wczytanie kosci wlasciwych dla szarfy.
- Reset: W momencie zatrzymania ruchu lub respawnu.
- Dealokacja: Niszczony razem z usunieciem szarfy lub usunieciem postaci.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):**
  - Wywolywany bezposrednio przez CPythonCharacterManager i CInstanceBase w momencie podpiecia szarfy do modelu.
  - Otrzymuje stan fizyki ruchu przez metody aktualizujace wektory przemieszczenia.
- **Zaleznosci wyjsciowe (Outbound):**
  - Granny 3D: modyfikacja transformacji lokalnych kosci szarfy.
  - DirectX 9: Przekazywanie stalych macierzy do shaderow (model shader pipeline) jesli uzywany jest skinning sprzetowy.
- **Drzewo dyrektyw #include:**
  - CInstanceBase.h
  - EterGrnLib/ModelInstance.h
  - EterLib/Render/ModelShaderPipeline.h
  Ryzyko zaleznosci cyklicznych ograniczone poprzez forward declarations.
- **Model pamieciowy:** Wymaga uzycia zoptymalizowanych wskaznikow na zaktualizowane tablice macierzy kosci. 

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)
- **Tabela Klas i Struktur:**
  - InstanceClothWingsController - Rozmiar ok. 128-256 bajtow. Klasa dziala na glownym watku, zarzadza aktualizacja macierzy.
  - ClothBoneData - Struktura przechowujaca stan fizyczny danej kosci (pozycja, predkosc).
- **Tabela Metod Publicznych:**
  - void UpdatePhysics(float fDeltaTime) - Przelicza logike fizyczna krok po kroku. Brak wartosci zwracanej, modyfikuje stan szarfy.
  - void SetWindForce(D3DXVECTOR3 vWind) - Ustawia globalna sile oddzialujaca na material. 
  - void AttachToInstance(CInstanceBase* pInstance) - Inicjalizuje zaleznosci, przypina kosci szarfy do kosci szkieletu postaci.
- **Pamieciowy Layout Struktur (Memory Layout & Offsets):**
  - Tablica wektorow pozycji fizycznych (offsets np. +0x10) wymaganych do symulacji.
  - Macierze Granny (offsets +0x40).

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Modul w pelni po stronie klienta, jednak widocznosc szarfy i podpiecie kostiumu podyktowane sa przez Game Server (GC Packet: kostium, opcode uwarunkowany struktura postaci, np. HEADER_GC_CHARACTER_UPDATE).
- **Metody Pythona (PyMethodDef):** Modul eksportuje funkcje widoczne m.in w module chrmgr:
  - chrmgr.SetSashEffect(vid, effect_id)
  - Format METH_FASTCALL dla optymalnego przetwarzania, argumenty sprawdzane bezpiecznie przez C-API.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Funkcje musza byc wywolywane z glownego watku gry, gdyz komunikacja z Granny 3D w tym trybie nie jest bezpieczna watkowo.
- **Potencjalne punkty awarii:** 
  - Jesli gracz nagle zostanie wyrejestrowany przed dokonaniem operacji matematycznych szarfy. Zawsze nalezy sprawdzac wskaznik na CInstanceBase.
  - Przepelnienie bufora w systemach z duza iloscia wierzcholkow tkaniny.
- **Zarzadzanie zasobami (RAII):** Zasoby macierzy sa zarzadzane automatycznie przez cykl zycia kontrolera - obiekty powiazane bezposrednio ze smart pointerami (std::unique_ptr) dla unikniecia wyciekow ramu i vramu w modelach Granny.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Odnalezc modyfikatory predkosci w InstanceClothWingsController.
  2. Dodac funkcje obliczeniowa do wyliczania zagniecec i odbic wiatru.
  3. Skompilowac ze zmodyfikowanymi naglowkami z EterGrnLib.
- **Jak debugowac i logowac:**
  - Uzywaj nowo dodanego standardu logowania C++23: EterBase::ModernLogger::Debug("Sash physical tick delta {}", fDelta).
  - Unikaj uzywania przestarzalego TraceError.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):**
  - Podepnij struktury do testow jednostkowych inicjujac mock instancje Granny (dummy models bez ladowania danych z VFS) za pomoca TEST_MOCK_D3D9. Sprawdz czy po symulacji z okreslonym deltatime sily fizyczne przemiescily kosci o prawidlowy offset.

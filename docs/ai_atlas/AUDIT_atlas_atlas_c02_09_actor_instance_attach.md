---
task_id: "atlas_c02_09_actor_instance_attach"
cluster: "ACT"
module_name: "CActorInstance - Podpinanie Ekwipunku i Rejestracja Czesci Ciala"
target_files:
- src/GameLib/ActorInstanceAttach.cpp
- src/UserInterface/InstanceControllers/InstanceEquipmentModelImpl.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c02_09_actor_instance_attach.md"
architecture_layer: "Maszyna Stanow Postaci i Aktorzy"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
Modul "CActorInstance - Podpinanie Ekwipunku i Rejestracja Czesci Ciala" odpowiada za dynamiczna zmiane i renderowanie elementow wyposazenia (broni, zbroi, wlosow) oraz przypinanie efektow wizualnych (np. dym, swiatlo) do kosci modelu 3D postaci (CActorInstance).
W klasycznej architekturze (w pliku ActorInstanceAttach.cpp), logika ta jest scisle zintegrowana z klasa postaci, bezposrednio ladujac i rejestrujac modele broni w zaleznosci od typu uzywanej broni (jednoreczna, dwureczna, sztylety, luk).
W nowej, zrefaktoryzowanej architekturze klienta m2dev-x64-2026, zarzadzanie tymi elementami przejeto przez dedykowane kontrolery instancji, takie jak IInstanceEquipmentModelController (wraz z implementacjami dla glownej broni, broni drugiej reki itd.), ktore rozdzialaja logike prezentacji od stanu gry.
Kod wywolywany jest glownie podczas zdarzen zakladania i zdejmowania ekwipunku (OnUpdate po otrzymaniu pakietu aktualizacji ekwipunku) oraz w petli rysowania (OnRender) przy przypisywaniu matryc kosci do modeli czesci ciala.
Przeplyw danych: Event zmian ekwipunku -> Kontroler ekwipunku (np. InstanceEquipOffhandWeapon) -> Przekazanie do modelu IInstanceEquipmentModelController -> Aktualizacja graficznej instancji postaci w warstwie renderowania. Cykl zycia obiektow obejmuje alokacje modeli 3D po przypisaniu vnum przedmiotu, a nastepnie zwalnianie ich i ukrywanie efektow podczas usuwania z ekwipunku.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** Zmiany sa inicjowane przez logike pakietow sieciowych (GC) obslugujacych zmiane ekwipunku postaci, zdarzenia z interfejsu (np. zakladanie przedmiotu) lub modyfikatory postaci. Zdarzenia (Events) za posrednictwem Core::EventBus.
- **Zaleznosci wyjsciowe (Outbound):** Modul polega na CEffectManager do kontrolowania swiecenia broni, CItemData i CItemManager do pobierania specyfikacji modeli przedmiotu, CRaceData do danych rasy i kosci przypiec. Nowe kontrolery komunikuja sie uzywajac wbudowanego EterBase::ModernLogger i systemu zdarzen z UserInterface::Core::EventBus.
- **Drzewo dyrektyw #include:** 
  - ActorInstanceAttach.cpp: "StdAfx.h", "EffectLib/EffectManager.h", "ActorInstance.h", "ItemData.h", "ItemManager.h", "RaceData.h", "WeaponTrace.h".
  - InstanceEquipmentModelImpl.cpp (Konceptualny plik referencyjny reprezentowany przez kontrolery m.in. w InstanceEquip_MainWeapon.cpp i InstanceEquip_OffhandWeapon.cpp): "../StdAfx.h", "IInstanceEquipmentModelController.h", "EterBase/StrongTypes.h", "EterBase/Result.h", "EterBase/ModernLogger.h", "UserInterface/Core/EventBus.h". Brak oczywistych naruszen relacji wzajemnych (cykli).
- **Model pamieciowy:** W starej strukturze wystepuja czyste wskazniki C (np. CItemData*, CGraphicThing*) oraz wektory (std::vector<CWeaponTrace*>) do przechowywania sladow broni. W nowym kodzie wprowadzono std::unique_ptr oraz typy bezpieczne dla bledu (EterBase::PacketResult, std::expected).

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Tabela Klas i Struktur:**
  - CActorInstance (fragment Attaching): Glowna klasa odpowiedzialna za logiczne i przestrzenne pozycjonowanie aktora. Watek: Glowny (Renderowanie).
  - IInstanceEquipmentModelController: Interfejs do modyfikacji czesci modelu (np. bron, wlosy, kostium).
  - InstanceEquipOffhandWeapon / InstanceEquip_MainWeapon: Kontrolery implementujace wiazanie modelu do lewej/prawej reki na podstawie ID encji i vnum. Watek: Glowny.
  - MainWeaponEquippedEvent / OffhandWeaponChangedEvent: Struktury zdarzen wysylanych za pomoca EventBus po podpieciu/odpieciu broni. Watek: Eventowy (najpewniej w watku glownym uzytkownika).
  - ModelPart: Enum okreslajacy rodzaj slotu (Main, Weapon, WeaponLeft, Hair, Sash, CostumeBody, itd.). Wektor bajtow (uint8_t).

- **Tabela Metod Publicznych:**
  - CActorInstance::AttachWeapon(DWORD dwItemIndex, DWORD dwParentPartIndex, DWORD dwPartIndex): Aktualizuje slot przedmiotu o podane vnum. Przypisuje model 3D do grafiki gracza.
  - CActorInstance::AttachEffectByID(...) -> DWORD: Mapuje efekt wizualny na dany komponent ciala w oparciu o CRC sciezki piku.
  - InstanceEquipOffhandWeapon::EquipOffhand(EterBase::ItemVnum vnum) -> EterBase::PacketResult<void>: Mapuje i podlacza model narzedzia reki dodatkowej poprzez wyslanie zdarzenia do silnika wyswietlania. 
  - IInstanceEquipmentModelController::SetPart(ModelPart part, EterBase::ItemVnum vnum) -> EterBase::PacketResult<void>: Logika ustawiania wskazanej czesci z mapowaniem w zaleznosci od przekazanego typu (vnum).

- **Pamieciowy Layout Struktur (Memory Layout & Offsets):** 
  W obiekcie postaci znajduje sie m.in. tablica m_adwPartItemID[CRaceData::PART_MAX_NUM], zapamietujaca VNUM (zwykle DWORD) przedmiotow zalozonych w specyficznych gniazdach renderowania, wektor m_WeaponTraceVector z zapisem szlakow uderzen, czy lista przylaczonych efektow m_AttachingEffectList uzywajaca zdefiniowanego typu struktury TAttachingEffect.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe:** Powiazanie glownie z opcodami Game->Client typu GC_CHARACTER_UPDATE (gdzie w paczce zawarte sa numery vnum zbroi, broni, wlosow) lub w opcodach zarzadzania ekwipunkiem (GC_ITEM_EQUIP). Modul dekoduje przekazane numery przedmiotow wywolujac np. EquipOffhand().
- **Metody Pythona (PyMethodDef):** Modul jest zazwyczaj ukryty przed czystymi skryptami, poniewaz skrypty dzialaja na chrmgr module (np. funkcje zwrotne do instancji gracza do obslugi zalozenia broni po stronie UI - uzywajace API postaci i CPythonCharacterManager).

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Poniewaz klasy te operuja bezposrednio na strukturach Direct3D (jak m_worldMatrix oraz widocznosc graficzna) musza byc wolane na watku glownym. Zmiany wyposazenia powinny zostac zsynchronizowane, aby nie wystapilo nieprawidlowe wyswietlanie w klatce pomiedzy procesowaniem a rysowaniem (np. mruganie starego modelu).
- **Potencjalne punkty awarii (Crash Points & Edge Cases):** Brak poprawnego vnum w systemie przedmiotow (powrot bledu ze wbudowanego mechanizmu). Niepoprawne identyfikatory encji wysylane w zdarzeniach i wskazywanie usunietych aktorow (NPE na wlascicielach). Brak zasobow modelu wywola odtworzenie NULL - powiazane z tym referencje do kosci w efekcie moga zgubcic referencje (TraceError "Cannot get Bone Index").
- **Zarzadzanie zasobami (RAII):** System posluguje sie zarzadzaniem recznym efektami (DettachEffect, __ClearAttachingEffect), podczas gdy nowsza struktura z C++23 w systemie Eventow wymaga zachowania wlasciwego cyklu zycia sluchaczy (listeners) unikajac wyciekow po smierci CActorInstance.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji:**
  1. Zaprojektuj i dodaj nowy slot ekwipunku w enum ModelPart (np. Pancerz dla Mounta).
  2. Implementuj nowy kontroler po stronie UserInterface/InstanceControllers/ dziedziczac interfejsu modela.
  3. Emituj i nasluchuj odpowiednie IEvent dla swojego typu obiektu uzywajac EventBus.
  4. Zainicjuj logike laczenia nowego modelu do odpowiedniej kosci rasy uzywajac CActorInstance::RegisterModelThing.
- **Jak debugowac i logowac:** Do przesledzenia wyposazenia uzywaj biblioteki wbudowanej logera EterBase::ModernLogger::Info. W ActorInstanceAttach.cpp korzystaj z logowania Tracef do zidentyfikowania poszukiwanej macierzy kosci lub odrzuconych vnumow.
- **Jak testowac bez interfejsu graficznego:** Izoluj pliki InstanceEquip_MainWeapon miedzy wbudowanym menadzerem wydarzen. Utworz "dummy" klase rasy i nasluchiwacza na MainWeaponEquippedEvent, po czym wyslij komende ekwipujaca mockowany vnum przez instancje kontrolera na rzecz weryfikacji. 

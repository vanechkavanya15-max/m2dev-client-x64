---
task_id: "atlas_c01_11_phase_game_guild_party"
cluster: "NET"
module_name: "Obsluga Pakietow Gildii i Druzyny (PhaseGame Guild & Party)"
target_files:
- src/UserInterface/PythonNetworkStreamPhaseGameGuild.cpp
- src/UserInterface/PythonNetworkStreamPhaseGameParty.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c01_11_phase_game_guild_party.md"
architecture_layer: "Silnik Sieciowy, Maszyna Faz i Protokol"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

### 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry"):
- **Funkcja modulu:** Modul ten odpowiada za obsluge sieciowa zjawisk zwiazanych z gildiami i grupami (party). Funkcjonuje jako "Mostek" (Bridge) miedzy C++ a Pythonem (`PhaseGameGuildBridge`, `PhaseGamePartyBridge`). Decyduje o interpretacji przychodzacych pakietow serwera od gier (G-C) dla gildii (np. przylaczenie do gildii, zalogowanie/wylogowanie, aktualizacja informacji o gildii) oraz grupie/party (np. zaproszenie do grupy, aktualizacja stanu hp grupy, nowe parametry dropu/pd itp). Odbywa sie to z naciskiem na bezpieczenstwo poprzez nowoczesne dekodery `C++23` z `std::span` i typem zwracanym w postaci `std::expected` (aliasing jako `PacketResult`).
- **Moment wywolania:** Kod wywolywany jest przez zarzadce sieci (`CPythonNetworkStream`) w fazie gry (Phase Game). Glowne punkty wejscia wywolywane sa przy odbiorze nieprzetworzonego pakietu (metody dispatchera).
- **Przeplyw danych (Control & Data Flow):** 
  1) `CPythonNetworkStream` odbiera bufor z gniazda.
  2) Seria instruksji `if/else` w duzym swtichu `PythonNetworkStreamPhaseGame.cpp` lub dispatch mapingu kieruje bufor na odpowiedni Bridge (np. `PhaseGameGuildBridge::HandleGuild` czy `PhaseGamePartyBridge::HandlePartyUpdate`).
  3) Codec (np. `Client::Network::PartyPacketCodec`) bezpiecznie odczytuje strukture (weryfikacja rozmiaru pakietu, return value `PacketResult<T>`).
  4) Jezeli udane, obiekty klienta (jak `CPythonPlayer`, `CPythonGuild`, czy `CPythonCharacterManager`) sa uaktualniane i wywolywana jest odpowiednia funkcja Pythona w warstwie GUI (np. `PyCallClassMemberFunc` -> `UpdatePartyMemberInfo`).
- **Cykl zycia obiektow:** Mostki `PhaseGameGuildBridge` oraz `PhaseGamePartyBridge` dzialaja bezstanowo jako metody statyczne, wiec nie sa instancjonowane ani dealokowane - pelnia jedynie funkcje fasady/routingu.

### 3. Dokladna Mapa Zaleznosci (Exact Dependency Map):
- **Zaleznosci wejsciowe (Inbound):** 
  - Glowny obslugujacy: `CPythonNetworkStream` z pliku `PythonNetworkStreamPhaseGame.cpp` wolajacy z dispatchera konkretne metody.
  - Oczekuje konkretnych pakietow z protokolu sieciowego (np. `TPacketGCPartyUpdate`, `TPacketGCGuildInfo`).
- **Zaleznosci wyjsciowe (Outbound):** 
  - `Client::Network::PartyPacketCodec` i `Client::Network::GuildPacketCodec` dla operacji odczytu (span + expected).
  - Obiekty zarzadcze gry: `CPythonPlayer`, `CPythonCharacterManager`, `CPythonGuild`, `CPythonMessenger` do przekazywania stanow.
  - Python C-API poprzez `PyCallClassMemberFunc` bezposrednio informujace UI o eventach.
- **Drzewo dyrektyw #include:** `StdAfx.h`, `PythonNetworkStreamPhaseGameParty.h`, `PythonNetworkStreamPhaseGameGuild.h`, `PythonNetworkStream.h`, `PythonPlayer.h`, `PythonGuild.h`, `PythonMessenger.h`, `PythonCharacterManager.h`, `Client/Network/PartyPacketCodec.h`, `Client/Network/GuildPacketCodec.h`, `<span>`. Ryzyko stanowia bezposrednie polaczenia interfejsu klienta do bezposrednio Python API przez co nie nalezy uzywac tych include w narzedziach "headless".
- **Model pamieciowy:** Dominuja przekazania referencyjne via referencje do stalej pakietow z codec'a. Uzywane sa tradycyjne, wylomowate (raw) wskazniki z obslugi `PythonApplication` i instancji z systemow w starym C++.

### 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index):
- **Klasa: PhaseGamePartyBridge**
  - *Rola:* Bezstanowy routing zdarzen party do systemu gracza.
  - *Wlasciciel:* Watek glowny.
  - *Metody:*
    - `static bool HandlePartyInvite(CPythonNetworkStream* pStream, const TPacketGCPartyInvite& pack)`: Reakcja na zaproszenie do party.
    - `static bool HandlePartyAdd(CPythonNetworkStream* pStream, const TPacketGCPartyAdd& pack)`: Dodanie uzytkownika do party (`PyCallClassMemberFunc` na `AddPartyMember`).
    - `static bool HandlePartyUpdate(CPythonNetworkStream* pStream, const TPacketGCPartyUpdate& pack)`: Aktualizuje zycie i buffy z party.
    - `static bool HandlePartyRemove(CPythonNetworkStream* pStream, const TPacketGCPartyRemove& pack)`: Usuwa czlonka party.
    - `static bool HandlePartyParameter(CPythonNetworkStream* pStream, const TPacketGCPartyParameter& pack)`: Reakcja na zmiane podzialu dropu (Distribution mode).
- **Klasa: PhaseGameGuildBridge**
  - *Rola:* Obsluguje gildie (routing do `CPythonGuild`).
  - *Metody:*
    - `static bool HandleGuild(CPythonNetworkStream* pStream, const uint8_t* pData, size_t size)`: Glowny parser dispatchera.
    - `static bool HandleGuildSub_Login(const TPacketGCGuild& pack)`: Informuje `CPythonMessenger` o zalogowaniu.
    - `static bool HandleGuildSub_Logout(const TPacketGCGuild& pack)`: Informuje o wylogowaniu.
    - `static bool HandleGuildSub_Info(const TPacketGCGuildInfo& info)`: Aktualizuje okno gildii (`CPythonGuild::GetGuildInfoRef`).
    - `static bool HandleGuildSub_Member(...)` i `HandleGuildSub_War(...)`: puste/stubbing - brak wlasnego event handlera.

### 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges):
- **Pakiety Sieciowe (Opcodes and Structs):** 
  - `TPacketGCPartyInvite` (header: GC::PARTY_INVITE)
  - `TPacketGCPartyAdd` (header: GC::PARTY_ADD)
  - `TPacketGCPartyUpdate` (header: GC::PARTY_UPDATE)
  - `TPacketGCPartyRemove` (header: GC::PARTY_REMOVE)
  - `TPacketGCPartyParameter` (header: GC::PARTY_PARAMETER)
  - `TPacketGCGuild` i podstruktury (`TPacketGCGuildInfo`, `TPacketGCGuildSubMember`, `TPacketGCGuildWar`) - headery sterowane przez enum z namespace `GuildSub::GC` (m.in LOGIN, LOGOUT, INFO).
- **Metody Pythona (wywolywane za pomoca `PyCallClassMemberFunc`):** 
  - `"RecvPartyInviteQuestion"` z arg `(is)` (pid, string nazwa).
  - `"AddPartyMember"` z arg `(is)`.
  - `"UpdatePartyMemberInfo"` z arg `(i)`.
  - `"RemovePartyMember"` z arg `(i)`.
  - `"ChangePartyParameter"` z arg `(i)`.

### 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas):
- **Zasady wielowatkowosci:** Wszystko musi sie odbywac na glownym watku klienta, poniewaz wywolania `PyCallClassMemberFunc` oraz bezposrednie operacje na GUI CPython API nie sa bezpieczne watkowo.
- **Potencjalne punkty awarii (Crash Points):** Nalezy sprawdzac wskazniki do pointerow (np `CInstanceBase* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(leader_pid);` - zweryfikowane przez `if(!pInstance)` w `HandlePartyInvite`). Brak pakietow party wywoluje awarie jesli wskaznik na klase dispatchera bylby nullowy (sprawdzane np `if(!pStream)`).
- **Zarzadzanie zasobami (RAII):** Kod dekodowania i deserializacji uzywa wektorow wartosci na stosie; nie ma manulanego malloc, bufor referencjonowany uzywa `std::span` by uniemozliwic wycieki.

### 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module"):
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):** 
  1. Dodaj strukture w `src/Client/Network/Protocol/Protocol.h`.
  2. Implementuj bezpieczny codec C++23 w `PartyPacketCodec.h/.cpp` / `GuildPacketCodec.h/.cpp` bazujacy na `std::span` i typie `PacketResult`.
  3. Zadeklaruj funkcje routingowa w odpowiednim Bridge (np. `PhaseGamePartyBridge`).
  4. Dodaj logike dispatchingu w `PythonNetworkStreamPhaseGame.cpp`.
  5. Zwroc do skryptu UI za pomoca `PyCallClassMemberFunc`.
- **Jak debugowac i logowac:** Uzywaj logow `TraceError` oraz `Tracef` w celach sledzenia, w klasach Codec bazuj na returnowaniach bledow unii `PacketError` - np `PacketError::BufferUnderflow`.
- **Jak testowac bez interfejsu graficznego:** Obowiazkowo pisz testy poslugujac sie mechanizmem mockingu izolujacym includy `StdAfx.h`. Buduj w srodowisku dummy via skrypt `run_isolated_test.sh` deklarujac makiety minimalne paczek C++ w `dummy_test_env` np odwolujac sie do `#ifndef ETERBASE_STDAFX_H` by uniknac uzycia D3D9.

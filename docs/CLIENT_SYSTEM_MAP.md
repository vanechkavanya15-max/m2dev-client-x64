# Kompendium Klienta: Nowoczesna Centralna Mapa Systemu (CLIENT_SYSTEM_MAP 2026)

## 1. Cel Architektoniczny i Rola Modulu

Centralna Mapa Systemu definiuje zintegrowana architekture klienta gry C++23 po pelnej dekonstrukcji historycznych monolitow kodu z lat 2004–2014. Klient operuje wedlug wzorca **AI-First Architecture** oraz **Data-Oriented Design (DOD)**:

- **Single Source of Truth**: Eliminacja globalnych zmutowanych buforow na rzecz domen [`WorldContext`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Core/WorldContext.h), [`InventoryDomain`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/InventoryDomain.h), [`SkillDomain`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/SkillDomain.h) i [`PlayerStatsDomain`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/PlayerStatsDomain.h).
- **Zbalansowane SRP (200–450 linii)**: Brak gigantycznych monolitow i brak antywzorca mikro-plikow (<50 linii).
- **Zero-Copy & Zero-Alloc**: Nowoczesny dispatch tablicowy $O(1)$ w [`ModernPacketDispatcher`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/ModernPacketDispatcher.h) z buforowaniem zero-copy (`std::span<const uint8_t>`).
- **Memory Safety & ABA Prevention**: Generacyjny rejestr encji [`GenerationalRegistry`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Actor/EntityHandle.h) oraz bezpieczny rejestr funkcyjny [`ActorRegistry`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/World/ActorRegistry.h) eliminujacy wiszace wskazniki i Use-After-Free.

---

## 2. Diagram Architektury Systemu (C++23 AI-First)

```mermaid
graph TD
    %% Glowny punkt wejscia
    WinMain[WinMain / UserInterface.cpp] --> AppInit[CPythonApplication::Initialize]
    WinMain --> MainLoop[Main Message Loop]
    MainLoop --> AppUpdate[CPythonApplication::Update]
    MainLoop --> AppRender[CPythonApplication::Render]
    
    %% Warstwa domenowa
    AppInit --> WorldCtx[Client::Core::WorldContext]
    WorldCtx --> InvDomain[InventoryDomain]
    WorldCtx --> SkillDomain[SkillDomain]
    WorldCtx --> StatsDomain[PlayerStatsDomain]
    WorldCtx --> ActorReg[ActorRegistry & SpatialHashGrid]
    
    %% Warstwa sieciowa
    AppUpdate --> NetDispatcher[ModernPacketDispatcher O(1)]
    NetDispatcher --> NetHandlers[117 Handlerow SRP + 4 Domeny Podsystemowe]
    NetHandlers --> NetEvents[Core::EventBus]
    NetEvents --> WorldCtx
    
    %% Mostek Pythona C-API
    AppInit --> PyPlayerModule[PythonPlayerModule Spis Tresci]
    PyPlayerModule --> PyInv[PythonPlayerModule_Inventory]
    PyPlayerModule --> PyCombat[PythonPlayerModule_Combat]
    PyPlayerModule --> PySkills[PythonPlayerModule_Skills]
    PyPlayerModule --> PyMove[PythonPlayerModule_Movement]
    
    %% Cykl zycia postaci
    AppUpdate --> InstanceBase[CInstanceBase Core]
    InstanceBase --> StranglerFacade[StranglerInstanceFacade]
    InstanceBase --> Appearance[InstanceBaseAppearance]
    InstanceBase --> Mount[InstanceBaseMount]
    StranglerFacade --> GenerationalReg[GenerationalRegistry O(1)]
    StranglerFacade --> ActorReg
```

---

## 3. Zdekonstruowane Monolity i Nowe Domeny

| Podsystem | Stan Pierwotny (Legacy) | Stan Zdekonstruowany (C++23 AI-First) | Kluczowe Klasy i Pliki |
|---|---|---|---|
| **Monolit Sieciowy** | `PythonNetworkStreamPhaseGame.cpp` (10 713 linii) | Tablicowy dispatch $O(1)$, zero-copy framing, handlery SRP | [`ModernPacketDispatcher.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/ModernPacketDispatcher.h), [`PartyPacketDomainHandler`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Handlers/PartyPacketDomainHandler.h), [`GuildPacketDomainHandler`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Handlers/GuildPacketDomainHandler.h), [`QuestDialogDomainHandler`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Handlers/QuestDialogDomainHandler.h), [`RefineExchangeDomainHandler`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Handlers/RefineExchangeDomainHandler.h) |
| **Monolit Gracza** | `PythonPlayer.cpp` (7 523 linie, `m_playerStatus`) | Domeny DDD, silne typowanie (StrongTypes), brak singletonu | [`InventoryDomain.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/InventoryDomain.h), [`SkillDomain.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/SkillDomain.h), [`PlayerStatsDomain.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Gameplay/PlayerStatsDomain.h), [`WorldContext.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Core/WorldContext.h) |
| **Monolit Aktora** | `InstanceBase.cpp` (5 732 linie, raw pointers) | Strangler Facade, GenerationalRegistry, SoA ECS | [`StranglerInstanceFacade.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Bridge/StranglerInstanceFacade.h), [`ActorRegistry.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/World/ActorRegistry.h), [`InstanceBaseAppearance.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/InstanceBaseAppearance.cpp), [`InstanceBaseMount.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/InstanceBaseMount.cpp) |
| **Struktury Protokolu** | `Protocol.h` (2 039 linii, monolit) | Fasada 19 linii + 5 odizolowanych naglowkow domenowych | [`Protocol_Common.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Protocol_Common.h), [`Protocol_Player.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Protocol_Player.h), [`Protocol_Item.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Protocol_Item.h), [`Protocol_Combat.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Protocol_Combat.h), [`Protocol_GuildParty.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/Client/Network/Protocol/Protocol_GuildParty.h) |
| **Bindingi Pythona C-API** | `PythonPlayerModule.cpp` (2 592 linie, 130 funkcji) | Podzial na 4 domeny SRP + glowny rejestr method table | [`PythonPlayerModule_Inventory.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonPlayerModule_Inventory.cpp), [`PythonPlayerModule_Combat.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonPlayerModule_Combat.cpp), [`PythonPlayerModule_Skills.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonPlayerModule_Skills.cpp), [`PythonPlayerModule_Movement.cpp`](file:///E:/m2dev-client-src-mainOryginalx64/src/UserInterface/PythonPlayerModule_Movement.cpp) |

---

## 4. Wytyczne dla Autonomicznych Agentow AI (AI-First Coding Standards)

1. **Budzet pliku**: Nowe moduly implementacyjne musza zawierac sie w przedziale **150–450 linii kodu**. ZABRANIA SIE tworzenia mikro-plikow ponizej 50 linii oraz monolitow powyzej 800 linii.
2. **Typowanie**: Zamiast surowych typow calkowitych (`DWORD`, `int`) nalezy bezwzglednie stosowac silne typy domenowe z [`StrongTypes.h`](file:///E:/m2dev-client-src-mainOryginalx64/src/EterBase/StrongTypes.h): `EntityVid`, `ItemVnum`, `SlotIndex`, `Money64`.
3. **Zarzadzanie encjami**: Zamiast bezposrednich wskaznikow `CInstanceBase*` agenci musza korzystac z uchwytow `EntityHandle` lub bezpiecznego wzorca funkcyjnego `ActorRegistry::VisitActor(vid, visitor)`.
4. **Weryfikacja**: Kazda zmiana musi zostac zweryfikowana uruchomieniem testow CTest (`ctest -C Release`). Wszystkie 40 testow musi miec status PASS.

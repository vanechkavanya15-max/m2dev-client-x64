---
task_id: "atlas_c06_10_light_manager"
cluster: "RND"
module_name: "CLightManager - Oswietlenie Swiata Gry i Zrodla Swiatla D3D9"
target_files:
- src/EterLib/GrpLightManager.cpp
- src/EterLib/GrpLightManager.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c06_10_light_manager.md"
architecture_layer: "Renderowanie, Direct3D 9, Shading i Culling"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# AUDIT RAPORT - Jules Swarm ATLAS (085/150)

## 1. Wstep i Kontekst Refaktoryzacji
Modul oswietlenia sceny i zrodel swiatla Direct3D 9 zostal zaimplementowany w klasach [`CLight`](file:///E:/m2dev-client-src-mainOryginalx64/src/EterLib/GrpLightManager.h) oraz [`CLightManager`](file:///E:/m2dev-client-src-mainOryginalx64/src/EterLib/GrpLightManager.h) w ramach biblioteki `EterLib`. Zastepuje on archaiczne, rozproszone zarzadzanie swiatlami, oferujac pule dynamiczna, sortowanie zrodel wedlug odleglosci od kamery oraz plynne przejscia barwne (blending).

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
`CLightManager` odpowiada za:
1. **Rejestracje i cykl zycia zrodel swiatla:** Obsluguje zarowno swiatla statyczne (`LIGHT_TYPE_STATIC`), jak i krotkotrwale efekty dynamiczne (`LIGHT_TYPE_DYNAMIC`, np. blyski magii, wybuchy).
2. **Sortowanie i limit sprzetowy D3D9:** Karta graficzna posiada ograniczona liczbe rejestrow swiatel sprzetowych (domyslnie limit `LIGHT_LIMIT_DEFAULT = 3` lub `D3DMAXTRACKEDLIGHTS`). `CLightManager` co klatke sortuje aktywne zrodla wedlug odleglosci od pozycji centralnej (`m_v3CenterPosition` - glowny gracz/kamera) i aktywuje w D3D9 wylacznie najblizsze zrodla.
3. **Plynna interpolacje barw:** `CLight::BlendDiffuseColor` oraz `BlendAmbientColor` obsluguja plynne wygaszanie i zapalanie swiatel w czasie rzeczywistym.
4. **Flush & Restore:** Metody `FlushLight()` oraz `RestoreLight()` umozliwiaja bezpieczne przelaczanie stanow swiatel dla specyficznych faz renderowania obiektow.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):**
  - `CEffectManager` i `CEffectInstance` (blyski czasteczek i magii).
  - `CMapOutdoor` i `CMapManager` (oswietlenie terenu, slonce, pochodnie).
  - `CPythonCharacterManager` (oswietlenie postaci i efektow aur).
- **Zaleznosci wyjsciowe (Outbound):**
  - `IDirect3DDevice9` (przez `CGraphicBase::ms_lpd3dDevice`).
  - `EterBase/Singleton.h` (`CSingleton<CLightManager>`).
  - `EterLib/Pool.h` (`CDynamicPool<CLight>`).
- **Drzewo dyrektyw `#include`:**
  - `GrpLightManager.h` includuje `EterBase/Singleton.h`, `GrpBase.h`, `Util.h`, `Pool.h`, `<deque>`, `<map>`, `<vector>`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Klasa `CLight`
- Dziedziczy: `CGraphicBase`, `CLightBase`
- Metody kluczowe:
  - `SetParameter(TLightID id, const D3DLIGHT9 & c_rLight)` - konfiguracja struktury D3D.
  - `SetDistance(float fDistance)` / `GetDistance()` - dystans od kamery dla algorytmu sortowania.
  - `SetDeviceLight(BOOL bActive)` - bezposrednie wywolanie `LightEnable()` w D3D.
  - `BlendDiffuseColor(...)`, `BlendAmbientColor(...)` - interpolacja barwy w czasie.

### Klasa `CLightManager`
- Dziedziczy: `CSingleton<CLightManager>`
- Metody zarzadzajace:
  - `RegisterLight(ELightType LightType, TLightID * poutLightID, D3DLIGHT9 & LightData)` - przydziela ID z puli `m_LightPool` i rejestruje zrodlo.
  - `DeleteLight(TLightID LightID)` - usuwa swiatlo i zwraca ID do kolejki `m_NonUsingLightIDDeque`.
  - `Update()` - aktualizacja stanow przejsc barwnych i sortowanie wektora `m_LightSortVector`.
  - `FlushLight()` - aktywacja posegregowanych swiatel w urzadzeniu D3D.
  - `RestoreLight()` - dezaktywacja swiatel po zakonczonym passie renderowania.

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- Modul dziala czysto po stronie klienta (Client-Side Rendering Engine).
- Sterowany posrednio przez zdarzenia otoczenia mapy (pora dnia z pakietu `TPacketGCEnvironment`) oraz efekty postaci.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
1. **Zasada Flush/Restore:** Kazde wywolanie `FlushLight()` przed renderowaniem geometrii MUSI miec odpowiadajace mu wywolanie `RestoreLight()` po zakonczeniu rysowania. Pominiecie spowoduje wyciek stanow oswietlenia na interfejs UI (np. ciemne napisy).
2. **Pula identyfikatorow TLightID:** Identyfikatory sa zarzadzane przez kolejke `m_NonUsingLightIDDeque`. Podwojne usuniecie tego samego `TLightID` prowadzi do korupcji mapy `m_LightMap`.
3. **Limit sprzetowy D3D:** Nie nalezy podnosic `m_dwLimitLightCount` powyzej sprzetowego limitu karty graficznej (`D3DCAPS9::MaxActiveLights`), ktory na starszym sprzecie moze wynosic 8.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Dodanie nowego dynamicznego zrodla swiatla:**
  ```cpp
  D3DLIGHT9 light{};
  light.Type = D3DLIGHT_POINT;
  light.Diffuse = D3DXCOLOR(1.0f, 0.8f, 0.2f, 1.0f);
  light.Position = D3DXVECTOR3(x, y, z);
  light.Range = 800.0f;
  TLightID lightId = 0;
  CLightManager::Instance().RegisterLight(LIGHT_TYPE_DYNAMIC, &lightId, light);
  ```
- **Testowanie bez GPU:** W trybie `--headless` metody `SetDeviceLight` sa bezpiecznie pomijane dzieki atrapom urzadzenia graficznego.

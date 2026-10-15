---
task_id: "atlas_c08_06_grp_text_render"
cluster: "UI"
module_name: "CGraphicTextInstance - Renderer Lancuchow Znakow i Obrysow"
target_files:
- src/EterLib/GrpText.cpp
- src/EterLib/GrpText.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_06_grp_text_render.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CGraphicText` i powiazany z nim `CGraphicTextInstance` odpowiedzialne sa za ladowanie, renderowanie i zarzadzanie wyswietlaniem ciagow znakowych 2D na warstwie UI klienta.
Modul ten dostarcza m.in. implementacje ladowania czcionek (oparta o wektory z `CGraphicFontTexture`), tworzenie konturow (outline), system kolorowania tekstu inline (`|cFFFF0000`), a takze obsluge wieloliniowych lancuchow, wyrownanie poziome i pionowe oraz hiperlacza (linki sieciowe).
Podczas petli glownej silnika (OnUpdate i OnRender), CGraphicTextInstance buduje listy renderowanych znakow (przygotowuje offsety, mapuje znaki na tekstury), co bezposrednio jest wyswietlane na ekranie (OnRender).
Klasy dzialaja na poziomie klienta, bedac wywolywane glownie przez UI bazujace na Pythonie.
Cykl zycia obiektow:
1. Alokacja poprzez pule CDynamicPool (ms_kPool) za pomoca statycznej metody New().
2. Inicjalizacja poprzez przydzielenie zasobu czcionki CGraphicText do m_roText.
3. Budowanie (Update) wywolywane z modulu interfejsu (lub leniwie w czasie renderingu) - tekst jest parsowany pod katem tagow kolorow, bidi, formatowania.
4. Renderowanie na polecenie UI, wywolujace instrukcje rysowania kwadratow ze wspolrzednymi UV uzywajac CGraphicFontTexture.
5. Dealokacja (Delete), ktora zwraca obiekt z powrotem do puli.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):** Modul interfejsu graficznego (np. PythonWindowManagerModule, PythonGraphicTextModule), element interfejsu w Pythonie, UI w grze (okna, przyciski, czat). Pakiety sieciowe nie bezposrednio wplywaja na te warstwe, ale dane pakietowe, takie jak nazwa postaci czy czat, sa pozniej rzutowane na `CGraphicTextInstance`.
- **Zaleznosci wyjsciowe (Outbound):** Modul renderowania (EterLib/Render), DirectX (bezposrednio obslugiwane przez GraphicFontTexture z EterLib/GrpFontTexture).
- **Drzewo dyrektyw `#include`:** 
  - `GrpText.h`: `#include "Resource.h"`, `#include "Ref.h"`, `#include "GrpFontTexture.h"`
  - `GrpText.cpp`: `#include "StdAfx.h"`, `#include "EterBase/Utils.h"`, `#include "GrpText.h"`
- **Model pamieciowy:** Instancje `CGraphicTextInstance` alokowane i zarzadzane w specjalistycznej puli pamieci `CDynamicPool`. Zalezny `CGraphicText` obslugiwany przez inteligentne wskazniki z liczeniem referencji `CRef<CGraphicText>` (typ TRef). Stringi przechowuje glownie za pomoca standardowego `std::string`. Kontenery wewnetrzne `std::vector` (np. offsety dla znakow).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Tabele Klas i Struktur
| Nazwa | Rola | Wlasciciel Watku |
|-------|------|------------------|
| `CGraphicText` | Obiekt zasobu (Resource) opakowujacy teksture czcionki. Odpowiada za wczytanie nazwy (np. Arial:12.fnt). | Watek glowny (UI/Render) |
| `CGraphicTextInstance` | Reprezentuje pojedynczy widzialny napis. Zleca renderowanie. Odpowiada za wlasciwosci (kolor, pozycja). | Watek glowny (UI/Render) |

### Metody Publiczne (wybrane)
**CGraphicText:**
- `static TType Type()`: Identyfikuje typ resource'u w managerze (zwraca wartosc bazujaca na hash z "CGraphicText").
- `bool CreateDeviceObjects()`: Deleguje tworzenie elementow czcionki w pamieci D3D do `m_fontTexture`.
- `void DestroyDeviceObjects()`: Sprzata po `m_fontTexture`.

**CGraphicTextInstance (bazujac na analizie pokrewnych klas EterLib):**
- `static CGraphicTextInstance* New()`: Zwraca wolna instancje z puli.
- `static void Delete(CGraphicTextInstance* pkInst)`: Cofa instancje do puli w celu dealokacji.
- `void Render(RECT* pClipRect)`: Odpowiada za samo rysowanie geometrii tekstu.
- `void Update()`: Odpowiada za przetworzenie lancucha wejsciowego na fizyczne, widzialne bloki znakow (kerning, kierunek bidi itp.).

### Pamieciowy Layout Struktur (Memory Layout & Offsets)
- `CGraphicText` dziedziczy z `CResource`, wiec jego layout rozpoczyna sie od VTable bazy i jej czlonkow (reference counter, hash), nastepnie offsety CGraphicFontTexture.
- `CGraphicTextInstance` posiada istotne tablice (vector) przechowujace logiczne znaki na wizualne (Bidi).
- Do celow inzynierii wstecznej - uwaga na liste pol: m_isUpdate, m_isCursor, m_isSecret itp (bit/byte properties). 

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Mostki sieciowe:** Brak bezposredniego mapowania zdefiniowanych tu pakietow sieciowych (jest to kod front-endowy). Czat (CPythonChat) jednak dostarcza gotowe ciagi, ktore trafiaja do CGraphicTextInstance.
- **Python C-API:** Obslugiwane przez `PythonGraphicTextModule` oraz `PythonWindowManagerModule`. API Pythona wystawia:
  - Stale enumeracje: np. `TEXT_HORIZONTAL_ALIGN_LEFT`, `TEXT_VERTICAL_ALIGN_TOP`.
  - Zarzadzanie obiektem poprzez struktury API: `PyTuple_GetTextInstance` wykorzystywane do operowania tekstem po stronie pythonowego UI.

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Funkcje modyfikujace parametry i renderujace (`Update`, `Render`) nie sa chronione mutexami i MUST be called z glownego watku renderujacego aplikacje/UI.
- **Crash Points / Null Deref:** Jesli resource (`CGraphicText`) padnie, nalezy bezpiecznie traktowac zapytania `Render`. Ochrona m_isUpdate zapewnia, ze puste lub niezainicjalizowane teksty nic nie rysuja. Brak walidacji wezelow m_roText przed zarysowaniem uzycia moze zakonczyc sie av/crash.
- **Pulapka Wydajnosci (Update flag):** `m_isUpdate` zabezpiecza przed nadmiarowym przeliczaniem (parsing) sformatowanego tekstu w kazdej klatce. Zmiana wejscia uzywa flagi `m_isUpdate = false`.
- **Wycieki Pamieci (RAII):** Zasoby powinnismy zwalniac przez statyczne `CGraphicTextInstance::Delete()` zamiast uzycia globalnego operatora `delete`, poniewaz zarzadzanie pamiecia odbywa sie w DynamicPoolu.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Dodawanie nowej wlasciwosci czcionki (np. cieni):** Dodaj zmienna (np. bool m_isShadow) w `CGraphicTextInstance`. Nalezy w `Update()` obsluzyc modyfikatory cienia i w `Render()` wywolac alternatywne obrysowanie cienia (rysuj z odstepem +1, +1, uzywajac modyfikatora alfy). Pamietaj o aktualizacji bindingu Pythona w PythonWindowManager.
- **Logowanie/Debugowanie:** Do sprawdzania jak dziala outline, uzywaj breakpointu w `CGraphicTextInstance::Render`. Loguj `m_v3Position` przy problemach z ukrytym tekstem.
- **Testowanie bez interfejsu (Headless):** Test wymaga mockowania puli `CDynamicPool`, i sztucznego ladowania zasobow z wlasnym parserem dla plikow .fnt bez D3D Device. Uzywaj instancji CGraphicText z wlasnym parserem, by sprawdzic wyrownanie (align) i przeliczanie szerokosci (GetWidth).

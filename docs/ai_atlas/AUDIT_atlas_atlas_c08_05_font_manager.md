---
task_id: "atlas_c08_05_font_manager"
cluster: "UI"
module_name: "CFontManager - Rasteryzator Czcionek TrueType i Atlasy Glifow"
target_files:
- src/EterLib/FontManager.cpp
- src/EterLib/FontManager.h
- src/EterLib/GrpFontTexture.cpp
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c08_05_font_manager.md"
architecture_layer: "Interfejs Uzytkownika, Okna i System Tekstu"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# CFontManager i CGraphicFontTexture - Analiza Architektury

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
Modul `CFontManager` w polaczeniu z `CGraphicFontTexture` stanowi uklad renderowania czcionek w kliencie gry Metin2, odpowiadajac za poprawne wczytywanie plikow TrueType (.ttf/.ttc) poprzez zewnetrzna biblioteke FreeType (FT2) oraz dynamiczne generowanie i zarzadzanie teksturami z glifami (tzw. atlasy glifow). 

**Przeplyw Danych i Cykl Zycia (Control Flow & Data Flow):**
1. Inicjalizacja: W fazie ladowania gry (zwykle w `UserInterface.cpp`), wywolywana jest metoda `CFontManager::Instance().Initialize()`, ktora inicjalizuje biblioteke FreeType (`FT_Init_FreeType`) oraz ustala filtrowanie LCD w celu unikniecia rozmycia kolorow na krawedziach. Mapowane sa takze standardowe sciezki do plikow z czcionkami.
2. Ladowanie Czcionki (Face): Kiedy inny komponent gry (np. `CGraphicText` czy `CTextBar`) potrzebuje okreslonej czcionki, przekazuje nazwe do `CFontManager::CreateFace`. Menedzer ustala sciezke dostepu (sprawdzajac cache, lokalny folder `fonts/` oraz folder systemowy `C:\Windows\Fonts`) i deleguje wczytywanie pliku do FreeType (`FT_New_Face`), zwracajac bezposrednio ow obiekt `FT_Face` (ktorym wylacznie zarzadza obiekt wywolujacy - alokacja ownership).
3. Zarzadzanie Glifami: Obiekt `CGraphicFontTexture` otrzymuje inicjalizacje dla okreslonej wielkosci oraz stylu (np. kursywa aplikowana za pomoca macierzy transformacji `FT_Set_Transform`). Rezerwuje bufor procesora (`m_pAtlasBuffer`) majacy typowo 256x256 lub 512x512 pikseli, ktory jest nastepnie zrzucany do GPU.
4. Generowanie Glifu: Kiedy potrzebne jest wyswietlenie glifu, ktorego nie ma w atlasie tekstur, wolane jest `UpdateCharacterInfomation`. Glif jest renderowany we FreeType (z `FT_RENDER_MODE_LCD`), a nastepnie nastepuje iteracja i mapowanie wynikowego bitowego pokrycia antyaliasingu pikseli (R,G,B w LCD) na odpowiedni odcien szarosci. Nastepnie dane kopiuja sie w wyznaczone miejsce atlasu na CPU i atlas oznaczany jest jako 'brudny' (`m_isDirty = true`).
5. Aktualizacja Atlasu na GPU (OnRender / Prerender): Przed narysowaniem tekstu wywolywane jest `UpdateTexture`, co lockuje teksture graficzna D3D i aktualizuje zaktualizowane rzedy bufora procesora do VRAM przy pomocy funkcji memcpy. W miare przepelnienia jednego bufora tekstury (brak miejsca na kolejne znaki), generowane sa kolejne `CGraphicImageTexture`, dodawane do wektora `m_pFontTextureVector` i nowa tekstura zaczyna wypelniac sie danymi.
6. Czyszczenie: Kiedy obiekty ulegaja zniszczeniu, `Destroy` dealokuje bufory, niszczy referencje w teksturach oraz oddaje pamiec, zwalniajac biblioteke za pomoca `FT_Done_Face` i ostatecznie `FT_Done_FreeType` w `CFontManager`.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)

**Zaleznosci wejsciowe (Inbound):**
- Menedzer jest globalnym singletonem inicjowanym podczas rozruchu aplikacji, glownie w `src/UserInterface/UserInterface.cpp`.
- `CGraphicFontTexture` jest stosowany w `CGraphicText` i przez jego potomkow (`CGraphicTextInstance`), sluzacych do rysowania tesktu w oknach interfejsu Pythona.
- Modyfikowany takze z poziomu UI przy generowaniu tekstow w np. `CTextBar`.

**Zaleznosci wyjsciowe (Outbound):**
- **FreeType (FT2):** Biblioteka odpowiada za parsowanie i wyliczanie metryk glifow. Zaleznosc wymaga dolaczenia `<ft2build.h>`, `FT_FREETYPE_H`, `FT_LCD_FILTER_H`.
- **D3D/EterLib:** Obiekty tekstur sa silnie sprzezone z podsystemem graficznym zdefiniowanym w `GrpTexture.h` (dziedziczy po `CGraphicTexture`) i `GrpImageTexture.h` co pociaga za soba wywolania D3D (np. D3DFMT_A8R8G8B8).
- **System Plikow:** Do znalezienia i zaladowania plikow fontu. Obejmuje standardowe odwolania do funkcji systemowych (`sys/stat.h`) oraz specyficzne pod system Windows (`windows.h`, `shlobj.h` sluzace do odpytania API Win32 `GetWindowsDirectoryA` by znalesc lokalizacje folderu z czcionkami systemowymi, poniewaz brak dostepnych alternatyw wielosystemowych np. fontconfig/X11, co wymusza portowanie tego kodu, by byl sprawny w pelni na Linuksie bez znieksztalcen).

**Drzewo dyrektyw `#include` i Ryzyka:**
- `FontManager.h` -> `<ft2build.h>`, `FT_FREETYPE_H`, `<string>`, `<unordered_map>`. (Rozsadne, bezpieczne dyrektywy standardowe).
- `GrpFontTexture.h` -> `GrpTexture.h`, `GrpImageTexture.h`, `<ft2build.h>`, `FT_FREETYPE_H`, `<vector>`, `<map>`.
- Brak groznych powiazan cyklicznych miedzy tymi plikami. Klasy trzymaja jedynie standardowe `std::vector` i `std::map` aby kontrolowac indeksy, jak i `TGraphicImageTexturePointerVector` oparty o czyste wskazniki `CGraphicImageTexture*`, ktore dziedzicza po API bezposrednio modyfikujacym pamiec D3D.

**Model pamieciowy:**
- Cache menadzera zaimplementowano na strukturach z STL (`std::unordered_map`), przetrzymujac wartosci typu string dla krotkich i elastycznych stringow.
- Struktury C++ w API uzywaja **czystych wskaznikow** (ang. raw pointers). Przyklad: bufor CPU `DWORD* m_pAtlasBuffer` do ktorego nastepuje dynamiczna alokacja tablicowa za pomoca `new DWORD[width * height]`. Obowiazkowe poprawne reczne zwolnienie uzywajac `delete[]`.
- Zwracany typ struktury metadanych znaku z atlasu to takze czysty wskaznik `TCharacterInfomation*`. Nalezy go odpowiednio walidowac przed uzyciem (czy nie jest nullptr).
- Same tekstury to recznie zarzadzany cykl pamieciowy D3D (`m_lpd3dTexture = NULL;` i inne czyste wskazniki na tekstury CGraphicImageTexture alokowane i kasowane operatorem delete).

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

**Tabela Klas i Struktur:**
| Nazwa Klasy/Struktury | Rola | Wielkosc/Layout | Wlasciciel Watku |
| --- | --- | --- | --- |
| `CFontManager` | Globalny Singleton (Menedzer). Inicjalizuje, wyszukuje z plikow i tworzy referencje czcionek dla procesow, bazujac na systemie operacyjnym badz lokalnym systemie plikow | Dynamiczna mapa cache | Glowny watek (Render/UI) |
| `CGraphicFontTexture` | Dziedziczy po `CGraphicTexture`. Obsluguje atlasowanie bitmap glifow i generowanie macierzy UV i tekstury D3D | N/A, dynamiczny bufor i wektor | Glowny watek (Render/UI) |
| `CGraphicFontTexture::SCharacterInfomation` | (Typedef `TCharacterInfomation`). Zwraca pozycje i rozmiar danego glifu (indeks atlasu, left, top, right, bottom, width, height, advance, bearingX). | 22 Bajty (2x short + 7x float) | Glowny watek |

**Tabela Metod Publicznych:**
| Sygnatura | Wartosc Zwracana | Opis / Efekty Uboczne (Pre/Post conditions) |
| --- | --- | --- |
| `static CFontManager& Instance()` | `CFontManager&` | Singleton zwracany przez referencje statyczna. Nalezy pamietac o poprawnej jednorazowej inicjalizacji i niszczeniu na koncu cyklu aplikacji. |
| `bool CFontManager::Initialize()` | `bool` | Odpala biblioteke `FT_Init_FreeType`. Pre: Wymaga wywolania podczas inicjalizacji. Post: Uzupelnia globalny cache lokalizacyjny plikow `.ttf`. |
| `FT_Face CFontManager::CreateFace(const char* faceName)` | `FT_Face` (czysty wskaznik C) | Alokuje i zwraca wskaznik wygenerowanej czcionki przez libFT2. **Wywolujacy staje sie wlascicielem tego obiektu i MUSI dealokowac przez `FT_Done_Face`.** Pre: Musi byc zdefiniowany menedzer. |
| `bool CGraphicFontTexture::Create(const char* c_szFontName, int fontSize, bool bItalic)` | `bool` | Re-alokuje i inicjalizuje bufor pamieci `m_pAtlasBuffer` dopasowany dla glifow danej czcionki. Pre: Zwraca falsz, jesli operacja `CreateFace` na FontManager nie powiodla sie. |
| `TCharacterInfomation* CGraphicFontTexture::GetCharacterInfomation(wchar_t keyValue)` | Wskaznik na informacje o glifie | Odpytuje cache tekstury o konkretny character. Jesli nie istnieje, wywoluje synchronicznie `UpdateCharacterInfomation` (ktory rasteryzuje z FreeType i zwraca wskaznik do mapy). |
| `bool CGraphicFontTexture::UpdateTexture()` | `bool` | Kopiuje bitmapy CPU do GPU (`pFontTexture->Lock(...)`). Pre: Musi istniec prawidlowy atlas CPU. Post: Ustawia zmienna `m_isDirty` = false. |

**Pamieciowy Layout Struktur (Memory Layout & Offsets):**
| Struktura | Kluczowe Pola (Typy) | Znaczenie i Offset dla Hookingu |
| --- | --- | --- |
| `SCharacterInfomation` | `index` (short) | Indeks wektora `m_pFontTextureVector` posiadajacy ten glif. (Offset: +0x0) |
| `SCharacterInfomation` | `width`, `height` (short) | Wymiary w pixelach tekstury atlasu dla znaku. (Offset: +0x2) |
| `SCharacterInfomation` | `left`, `top`, `right`, `bottom` (float) | Wymiary normalizowane do macierzy UV tekstury. Sluzy do budowania prostokata glifu dla mapy tekstu (RenderPrimitive). (Offset: +0x6) |
| `SCharacterInfomation` | `advance`, `bearingX` (float) | Informacje typograficzne niezbedne do prawidlowego dystansowania (kerning) poziomego liter i slow (spacing), np. dla cieni pod tekstem i formatowania tekstu. (Offset: +0x16) |

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- **Pakiety Sieciowe:** Modul ten jest czescia infrastruktury wyswietlania (Render Pipeline, UI), i jest niezalezny od warstwy sieciowej (Network Protocol, NetworkStream, Pakiety CG/GC). Nie posiada bezposredniego wejscia/wyjscia TCP i protokolu pakietowego.
- **Metody Pythona (`PyMethodDef`):** Modul jest silnie wykorzystywany przez mechanizm interfejsu (tj. GrpText, PythonTextTail i paski tytulow Python UI), ale same te klasy nie eksponuja wlasnych, unikalnych instancji Python C-API. Integracja nastepuje tylko i wylacznie poprzez obiekt nadrzedny tworzony z okien (`ui.py`).

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
- **Zasady wielowatkowosci:** Wszystkie odwolania do generowania nowych glifow (tj. caly modul FreeType i blokady sprzetowe tekstur DirectX `UpdateTexture`) zachodza synchronicznie glownym watkiem D3D. Rasteryzacja nie jest odlozona asynchronicznie, tak wiec jesli nastepuje wywolanie `GetCharacterInfomation` z nowym znakiem chinskim/koreanskim niespotykanym dotad w mapie `m_charInfoMap`, nastepuje spadek FPS wywolany na biezaco przez lockownie GPU i memory-cpy bufora CPU.
- **Potencjalne punkty awarii (Crash Points & Edge Cases):**
  1. Funkcja `UpdateTexture()` zaklada rozmiar piku bitowego 32 bity (A8R8G8B8) dzielac pitch przez 4: `pitch /= 4;`. Zmiana formatu z powrotem na inne kodowanie (np. A4R4G4B4) doprowadzi do calkowitego zawieszenia systemu (Out-of-Bounds memory overwrite) przez niedokladnosc pamieci `pdwDst` w obrebie rzedu (ang. pitch/stride mismatch). 
  2. Mapowanie folderu czcionek Win32: funkcja `GetWindowsDirectoryA` nie istnieje w API Linux, stad portowanie tego silnika na obiekty systemowe takie jak macOS, Unix, czy Linux wymaga specjalnego stub-u API sluzacego za powloke dostarczajac reczna sciezke dla folderow systemowych np. z fontconfig/X11, co wymaga nadpisania badz mockowania kodu dla stabilnych wirtualnych srodowisk w procesie testowania/kontenerow AI.
- **Zarzadzanie zasobami (RAII):** Kod mocno opiera sie na konwencjonalnym "wlasnym" systemie zarzadzania zasobami. W przypadku zwrotu z `CFontManager::CreateFace`, zasoby MUSZA byc zwolnione zewnetrznie przez klienta po przez `FT_Done_Face` co lamie bezpieczenstwo RAII oparte o standardowe zasady inteligentnych wskaznikow (np. `std::unique_ptr` wraz z customowym niszczycielem do FT_Face moglby wyeliminowac cala powierznie potencjalnego memory leak'u). Zobacz wywolanie w `CGraphicFontTexture::Destroy`. Wskaznik bufora CPU `DWORD* m_pAtlasBuffer` zarzadzany manualnie poprzez new/delete[], co zwieksza ryzyko jesli braknie exception handlera.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Instrukcja dodawania nowej funkcji (Step-by-step extension guide):**
  1. Jezeli zamierzasz wprowadzic nowy wariant formatu tekstury (np. DXT, PVR), zaktualizuj logike podzialu (pitch/4) w funkcji `UpdateTexture` i zmien wektor zapisu 32-bitowego dla poszczegolnych subpixeli i anty-aliasingu w funkcji `UpdateCharacterInfomation`.
  2. W razie wycieku pamieci (memory-leak'ow) spowodowanych tworzeniem niezabezpieczonych struktur bibliotek, zmodyfikuj interfejs w `CFontManager.h` owijajac `FT_Face` we wlasna polimorficzna klase opakowujaca (Wrapper RAII) badz stosujac `std::shared_ptr`/`std::unique_ptr` z niszczycielem wywolujacym obowiazkowo `FT_Done_Face`.
- **Jak debugowac i logowac:** Do przesledzenia przeplywu, nalezy zwracac szczegolna uwage na bufor m_charInfoMap za pomoca Visual Studio lub GDB. By upewnic sie, ze nie powstaja memory leaks'y w atlasie, zaleca sie umieszczenie logow (np. przez klase EterBase) w warunkach tworzenia nowych tekstur i wywolan `AppendTexture`, zrzucajac przy tym zawartosc np. `m_atlasWidth` do logow gdy w atlasie braknie miejsca, i koniecznosci przypinania na GPU kolejnej struktury CGraphicImageTexture.
- **Jak testowac bez interfejsu graficznego (Headless / Unit Test Harness):** W przypadku wdrozenia na kontener Linux, wywolanie funkcji opartych o `_WIN32` musi zostac zakryte przez odpowiednie flagi w dyrektywie `#ifdef`. Testy z mockowaniem CGraphicImageTexture umozliwia testowanie calkowicie w isolation bufora CPU: symulujac jedynie obsluge alokacji FT2 na tablicy `m_pAtlasBuffer` poprzez wpisanie np. czcionki recznej do cache z pliku wzorcowego a nastepnie sprawdzenie tablicy z przewidywanym buforem binarnym referencyjnym (snapshot testing) z pominieciem warstwy sterownika wyswietlacza Windows (DirectX/Granny3D).

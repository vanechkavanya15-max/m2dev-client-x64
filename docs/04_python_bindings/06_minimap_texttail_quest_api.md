# Modul UserInterface: miniMap, textTail oraz quest

## 1. Cel Architektoniczny i Rola Modulu

W kliencie Metin2 podsystem interfejsu uzytkownika realizowany jest glownie poprzez biblioteke EterPythonLib we wspolpracy z C++ `UserInterface`. Trzy kluczowe moduly graficzno-informacyjne dla gracza to:

*   **miniMap (CPythonMiniMap)**: Modul renderujacy zarowno okragla minimape na ekranie (w tym znaczniki zalezne od typu, polozenia, rotacji), jak i pelnoekranowa mape (Atlas). Modul rzutuje pozycje z 3D (`CMapOutdoor`) do lokalnego ukladu na mapie, poslugujac sie shaderami lub podstawowymi transformacjami `CTerrain` do nakladania maski (MiniMapFilter). Zalezy bezposrednio od modulu zewnetrznych danych terenu.
*   **textTail (CPythonTextTail)**: System rzutujacy teksty i obiekty 2D nad instancjami 3D postaci i przedmiotow (tzw. "chmury" nad glowami). Zarzadza paskami zycia, nazwami gildii, tytulami (PK), nazwami NPC oraz dropnietymi przedmiotami. Opiera sie na funkcji rzutowania przez kamere z 3D na wspolrzedne ekranu i sortuje teksty, aby zapobiec nakladaniu sie.
*   **quest (CPythonQuest)**: Prosty model danych dzialajacy jako Single Source of Truth dla instancji questow z aktywnymi zegarami (odliczaniem w dol lub w gore). Wspolpracuje m.in. z modulem Python, dostarczajac narzedzi GUI do wyrysowania "Zwoju" lub ikon z lewej strony ekranu, ktory reaguje na zamykanie (Clear) czy tworzenie zadania.

Zaleznosci to w duzej mierze podsystem C++ `EterLib` (dla klas bazowych, ladowania obrazow `CGraphicExpandedImageInstance`), `GameLib` (obiekty postaci), zas komunikacja z interpreterem Python zachodzi przy uzyciu `PyMethodDef` API (biblioteka standardowa C).

## 2. Diagram Architektury i Przeplywu Danych (Mermaid)

```mermaid
graph TD
    Python[Python UI Scripts]
    subgraph UI Modules
        MiniMap[CPythonMiniMap]
        TextTail[CPythonTextTail]
        Quest[CPythonQuest]
    end
    Network[Network / Packet Handler]
    GameLib[GameLib: Character/Item instances]
    EterLib[EterLib: CGraphicImage, TextInstance]
    MapOutdoor[MapOutdoor: CTerrain]
    
    Python -->|Py_InitModule, C-API Calls| UI Modules
    Network -->|Add Waypoints, Quest Updates| UI Modules
    MiniMap -->|Get mini map texture| MapOutdoor
    TextTail -->|Transform 3D to 2D| GameLib
    TextTail -->|Pool Allocations| EterLib
    Quest -->|Get icon from ResourceMgr| EterLib
```

## 3. Rejestr Struktur Danych i Pamieci (Memory & Struct Layout)

### CPythonMiniMap

#### `enum (anonimowy #1)`
*   `EMPIRE_NUM = 4`: Liczba imperiow (krolestw).
*   `MINI_WAYPOINT_IMAGE_COUNT = 12`: Zdefiniowana statycznie liczba obrazkow malych waypointow.
*   `WAYPOINT_IMAGE_COUNT = 15`: Liczba obrazkow duzych waypointow.
*   `TARGET_MARK_IMAGE_COUNT = 2`: Liczba znacznikow celu.
*   *Wyrownanie/Pamiec*: Typowanie standardowe (zazwyczaj 4-bajtowy `int`).

#### `enum (anonimowy #2)`
*   Typy obiektow i znacznikow dla minimapy: `TYPE_OPC`, `TYPE_OPCPVP`, `TYPE_OPCPVPSELF`, `TYPE_NPC`, `TYPE_MONSTER`, `TYPE_WARP`, `TYPE_WAYPOINT`, `TYPE_PARTY`, `TYPE_EMPIRE`, `TYPE_EMPIRE_END`, `TYPE_TARGET`, `TYPE_COUNT`.
*   *Wyrownanie/Pamiec*: Typowanie standardowe (4-bajtowy `int`).

#### `TAtlasMarkInfo` (struct)
*   `m_byType` (`BYTE`): Typ znacznika ze zdefiniowanych wyzej enumeracji.
*   `m_dwID` (`DWORD`): Identyfikator wewnetrzny uzywany m.in. dla WayPoint.
*   `m_fX`, `m_fY` (`float`): Pozycja globalna z serwera.
*   `m_fScreenX`, `m_fScreenY` (`float`): Przeliczona pozycja na ekranie do wyrenderowania Atlasu.
*   `m_fMiniMapX`, `m_fMiniMapY` (`float`): Przeliczona pozycja do renderowania na okraglej minimapie.
*   `m_dwChrVID` (`DWORD`): Obiekt VID (Virtual ID) przypisany do wpisu (do biezacej aktualizacji polozenia postaci).
*   `m_strText` (`std::string`): Tekst dymku, tooltip markera.
*   *Wyrownanie (Packing)*: Standardowe alokowanie klas kompilatora C++ w Metin2 (brak pragma pack). Rozmiar okolo 40-44 bajtow na x86, zalezy od implementacji std::string.

#### `TGuildAreaInfo` (struct)
*   `dwGuildID` (`DWORD`): Identyfikator gildii zajmujacej dany teren.
*   `lx`, `ly`, `lwidth`, `lheight` (`long`): Dane serwerowe strefy gildii na mapie.
*   `fsxRender`, `fsyRender`, `fexRender`, `feyRender` (`float`): Cache obliczonego prostokata uzywany przy renderingu UI (Atlasu).
*   *Wyrownanie*: Domyslne x86 (4 bajty), rozmiar calkowity 36 bajtow.

#### `SObserver` (struct)
*   `fCurX`, `fCurY`, `fSrcX`, `fSrcY`, `fDstX`, `fDstY` (`float`): Pola interpolacji ruchu obserwowanego obiektu na minimapie (zrodlo, cel i obecna pozycja wyliczana z czasem).
*   `dwSrcTime`, `dwDstTime` (`DWORD`): Timestamps systemowe.
*   *Wyrownanie*: Domyslne (rozmiar 32 bajty).

#### `TMarkPosition` (struct)
*   `m_fX`, `m_fY` (`float`): Wspolrzedne polozenia markera dla potworow/NPC.
*   `m_eNameColor` (`UINT`): Kolor znacznika (powiazany z relacja wobec gracza, np. wrog czy przyjaciel).
*   *Wyrownanie*: Domyslne (rozmiar 12 bajtow).

#### `TSignalPoint` (struct)
*   `v2Pos` (`D3DXVECTOR2`): Wektor dwuwymiarowy polozenia sygnalu (ping).
*   `id` (`unsigned int`): Unikalne ID pingu sygnalowego.
*   *Wyrownanie*: Domyslne (rozmiar 12 bajtow).

### CPythonTextTail

#### `STextTail` / `TTextTail` (struct)
*   `pTextInstance`, `pOwnerTextInstance`, `pGuildNameTextInstance`, `pTitleTextInstance`, `pLevelTextInstance` (`CGraphicTextInstance*`): Wskazniki na silnik EterLib rysujacy sam tekst (etykiety 2D).
*   `pMarkInstance` (`CGraphicMarkInstance*`): Znacznik ikony logotypu gildii.
*   `pOwner` (`CGraphicObjectInstance *`): Obiekt docelowy, rzutowany jako cel kamery z GameLib.
*   `dwVirtualID` (`DWORD`): Unikalne ID obiektu-wlasciciela w grze.
*   `x`, `y`, `z` (`float`): Przetransformowana wspolrzedna na UI.
*   `fDistanceFromPlayer` (`float`): Wyliczona odleglosc w 3D - krytyczna dla Z-sorting dymkow na ekranie.
*   `Color` (`D3DXCOLOR`): Aktualna barwa podstawowego tekstu.
*   `bNameFlag` (`BOOL`): Informuje czy dla TextTail powiazanego z ChatTail'em wymagane jest dorysowanie nicku gracza.
*   `xStart`, `yStart`, `xEnd`, `yEnd` (`float`): Krawedzie (Bounding box) 2D sluzace do click-pickingu i systemu ArrangeTextTail (by dymki nie nakladaly sie jeden na drugi).
*   `LivingTime` (`DWORD`): Czas, po ktorym dymek chatu samoistnie zniknie.
*   `fHeight` (`float`): Korekta wzniosu dla BB w pionie, by ustapic miejsca dla rosnacej ilosci innych TextTailow w tym samym rejonie.
*   *Wyrownanie*: Alokowane z uzyciem mechanizmu z dynamic pool z bibliotek EterBase (`CDynamicPool<STextTail>`). Standardowe packowanie (rozmiar zalezny od wskaznikow, na x86 ok. 76-80 bajtow).

### CPythonQuest

#### `SQuestInstance` (struct)
*   `dwIndex` (`DWORD`): Unikalny numer instancji zadania od strony serwera.
*   `strIconFileName`, `strTitle`, `strClockName`, `strCounterName` (`std::string`): Nazwy wyrysowywane w Pythonie (UIQuest) i plik ikony (TGA).
*   `iClockValue`, `iCounterValue` (`int`): Stany wewnetrznych znacznikow questu (czas do konca, albo ilosc wymaganych potworow).
*   `iStartTime` (`int`): Czas utworzenia w sekundach sluzacy jako wzorzec startowy dla odliczen `GetQuestLastTime`.
*   *Wyrownanie*: Domyslne, ze wzgledu na obecnosc az 4 klas std::string, znaczaco przekracza 100 bajtow.


## 4. Rejestr Klas i Metod (API Reference)

### CPythonMiniMap

*   `CPythonMiniMap()`: Konstruktor. Zeruje macierze DirectX, wywoluje wewnetrzne `__Initialize()` dla pustych wektorow map.
*   `~CPythonMiniMap()`: Destruktor, czysci wskazniki na textury uzywajac wbudowanego `Destroy()`.
*   `void Destroy()`: Calkowicie zwalnia wszystkie wskazniki do grafik maskujacych (Atlas, MiniWayPoints) z bufora VRAM EterLib. Zwalnia vertex i index bufory.
*   `bool Create()`: Buduje CGraphicVertexBuffer, ustawia CGraphicIndexBuffer pozwalajacy pozniej wyrenderowac biale punkciki przez grafike bez wielokrotnego rysowania od zera.
*   `bool IsAtlas()`: Sprawdza flage `m_bAtlas` czy gra zezwala na rysowanie pelnej mapy M.
*   `bool CanShow()`: Bada czy okragla minimapa ma uprawnienia (np. `m_bShow` aktywny w zaleznosci od interfejsu klienta).
*   `bool CanShowAtlas()`: Bada `m_bShowAtlas`, by zdecydowac w cyklu Render, czy Atlas jest do wyrysowania.
*   `void SetMiniMapSize(float fWidth, float fHeight)`: Przypisuje globalne rozmiary ramki UI, ustawia promien rysowania.
*   `void SetScale(float fScale)`: Przypisuje wspolczynnik powiekszenia maski okraglej do zmiennej `m_fScale`.
*   `void ScaleUp()`: Inkrementuje `m_fScale` o ustalony wspolczynnik. 
*   `void ScaleDown()`: Dekrementuje `m_fScale` o ustalony wspolczynnik.
*   `void SetCenterPosition(float fCenterX, float fCenterY)`: Przelicza wspolrzedne swiatowe mapy z `CMapOutdoor` na wewnetrzne siatki komorek UI, aby pobrac poprawna texture z `CTerrain`.
*   `void Update(float fCenterX, float fCenterY)`: Metoda ramki UI iterujaca struktury (NPC, wrogowie, teleporty). Pobiera referencje do instancji z GameLib i zapelnia dynamiczne obiekty `TMarkPosition` dla rysowania.
*   `void Render(float fScreenX, float fScreenY)`: Centralny punkt rysujacy D3D. Zapisuje D3DRS_TEXTUREFACTOR do Statemanagera, modyfikuje macierz kamery `m_matMiniMapCover`, oraz na samym koncu naklada ikonki powracajac do uprzedniego stany textur.
*   `void Show()`: Przelacza wartosc wyrysowywania `m_bShow` na true, udostepniajac ja na ekranie w grze.
*   `void Hide()`: Przelacza wartosc `m_bShow` na false.
*   `bool GetPickedInstanceInfo(float fScreenX, float fScreenY, std::string & rReturnName, float * pReturnPosX, float * pReturnPosY, DWORD * pdwTextColor)`: Oblicza relatywne odbicie kursora x/y przez cos/sin, zwracajac do C++ dane (np. kogo zaznaczyl gracz i jaka ma on flage nazwy/koloru).
*   `bool LoadAtlas()`: Bada wielkosc tekstury serwerowego obrysu (Atlasu z folderow sezonu/map) z bazy wirtualnego systemu EterPack.
*   `void UpdateAtlas()`: Aktualizuje stany widocznosci stref gildijnych wewnatrz Atlasu dla podsystemu aktualizacji renderera.
*   `void RenderAtlas(float fScreenX, float fScreenY)`: Skaluje grafiki duzej mapy w 2D na wspolrzednych podanych na wejsciu. Tworzy wlasne rysowania prostokatow `TGuildAreaInfo` na ukladzie kwadratu.
*   `void ShowAtlas()`: Uwidacznia pelnoekranowa Mape dla uzytkownika klienta gry.
*   `void HideAtlas()`: Cofa `m_bShowAtlas` do stanu falszu.
*   `bool GetAtlasInfo(float fScreenX, float fScreenY, std::string & rReturnString, float * pReturnPosX, float * pReturnPosY, DWORD * pdwTextColor, DWORD * pdwGuildID)`: Oblicza picking-hit w obszarze duzej grafiki pelnej mapy, sprawdzajac Bounding Box czy koliduje z kursorem myszki. Zwraca dane znacznika pod myszka.
*   `bool GetAtlasSize(float * pfSizeX, float * pfSizeY)`: Zwraca szerokosc i wysokosc dla AtlasImageInstance do biblioteki Pythona w warstwie C-API.
*   `void AddObserver(DWORD dwVID, float fSrcX, float fSrcY)`: Inicjalizuje strukture interpolacji obiektu w wektorze z identyfikatorem gracza.
*   `void MoveObserver(DWORD dwVID, float fDstX, float fDstY)`: Aktualizuje w interpolowanym celu biezace X i Y dla wyrenderowania smoothingu na minimapie.
*   `void RemoveObserver(DWORD dwVID)`: Usuwa wirtualne ID klienta ze wskaznikow obserwowanych instancji graczy w teamie (party).
*   `void AddWayPoint(BYTE byType, DWORD dwID, float fX, float fY, std::string strText, DWORD dwChrVID)`: Przylacza strukture kompasu jako TAtlasMarkInfo na podstawie wejsciowego zapytania po otrzymaniu paczki z sieci (lub z misji z wektora Python).
*   `void RemoveWayPoint(DWORD dwID)`: Usuwa wpis drogowskazu misji kompasu poslugujac sie jego systemowym identyfikatorem.
*   `void AddSignalPoint(float fX, float fY)`: Wstawia nowy ping (z komendy lub z teamu w grze) na mapie ze struktury typu wektor D3D2.
*   `void ClearAllSignalPoint()`: Wymazuje strukture z pamieci pod wzgledem starych sygnalow kropki swiecacej.
*   `void RegisterAtlasWindow(PyObject* poHandler)`: Laczy callback obslugi eventow Pythona ui.py dla wlasciwego widgetu UI.
*   `void UnregisterAtlasWindow()`: Usuwa zapamietany wskaznik PyObjecta uwalniajac pamiec.
*   `void OpenAtlasWindow()`: Wysyla poprzez CallClassMemberFunc Pythona metode "Show" po stronie scriptu UI.
*   `void SetAtlasCenterPosition(int x, int y)`: Centralizuje mape by okno skupialo uwage gracza na podanych wspolrzednych Atlasu.
*   `void ClearAtlasMarkInfo()`: Oproznia wektor markow dla calego Atlasu M.
*   `void RegisterAtlasMark(BYTE byType, const char * c_szName, long lx, long ly)`: Alokuje stringa z nazwa instancji do rzutu na mape w C++ (z wykorzystaniem TAtlasMarkInfo).
*   `void ClearGuildArea()`: Usuwa poprzedni layout stref na duzej mapie gildii.
*   `void RegisterGuildArea(DWORD dwID, DWORD dwGuildID, long x, long y, long width, long height)`: Zapisuje wymiary strefy dla gildii przeliczajac bezposrednio jej Bounding Boxy by przyspieszyc cykl Render.
*   `DWORD GetGuildAreaID(DWORD x, DWORD y)`: Zwraca identyfikator klanu dla wspolrzednych X/Y uzywajac stref HitBox z GuildAreaInfoVector.
*   `void CreateTarget(int iID, const char * c_szName)`: Powoluje proste wyrysowywanie ikon strzalek kierunku podanego celu.
*   `void CreateTarget(int iID, const char * c_szName, DWORD dwVID)`: Odmiana przypisujaca sledzenie celu wzgledem identyfikatora wrogiej lub przyjaznej instancji.
*   `void UpdateTarget(int iID, int ix, int iy)`: Aktualizuje koordynaty biezace celownika misji.
*   `void DeleteTarget(int iID)`: Zdejmuje obiekt graficzny targetu usuwajac odniesienie z kontenerow po id w zadaniu.
*   `void __Initialize()`: Sluzy w konstruktorze jako pomocnik resetujacy wektory i podstawowe flagi bool na false.
*   `void __SetPosition()`: Oblicza bezwzgledna pozycja MiniMapScreen uwzgledniajac srodek ekranu monitora i padding ramki uzytkownika.
*   `void __LoadAtlasMarkInfo()`: Pobiera liste prekonfigurowanych flag ze skryptu python uzywajac dostepnych narzedzi z plikow gry.
*   `void __RenderWayPointMark(int ixCenter, int iyCenter)`: Skupia punkt rysowania z wizerunkiem odpowiedniej instancji rozszerzonej CGraphic dla glownego duzego punktu trasy misji.
*   `void __RenderMiniWayPointMark(int ixCenter, int iyCenter)`: Rysuje mniejsza ikone na okraglej obwiedni w UI glownej z rotacja D3D na naroznikach minimapy dla celow misji, gdy przekraczaja obrys okna minimapy (pokazuje tylko kierunek celu).
*   `void __RenderTargetMark(int ixCenter, int iyCenter)`: Rysuje znak mrugajacy nad glowa podanego identyfikatora z celu misji.
*   `void __GlobalPositionToAtlasPosition(long lx, long ly, float * pfx, float * pfy)`: Transformuje swiatowa pozycje CMapOutdoor w 3D serwera z powrotem na relatywna, lokalna wielkosc okienka 2D.
*   `bool __GetWayPoint(DWORD dwID, TAtlasMarkInfo ** ppkInfo)`: Zwraca prawde i wypelnia referencje jesli zdefiniowany kompas wystepuje.
*   `void __UpdateWayPoint(TAtlasMarkInfo * pkInfo, int ix, int iy)`: Zamienia pola wpisu uaktualniajac jego biezaca rzutowana pozycje 2D w systemie minimapy.

### CPythonTextTail

*   `CPythonTextTail()`: Inicjalizuje mechanizm TextTail. Buduje nowa instancje dla textow i nadaje capacity pool.
*   `~CPythonTextTail()`: Klasyczny destruktor - czysci bufory, uwalnia obiekty w pool i niszczy texttailmapy by zapobiec memory leake'om.
*   `void GetInfo(std::string* pstInfo)`: Wpisuje zrzut z raportu diagnostycznego dla debugu gry do przekazanego bufora logow string.
*   `void Initialize()`: Przygotowuje podsystem poola wywolujac clear na pamieci zeby miec pusty uklad startowy map okien.
*   `void Destroy()`: Likwiduje cala mape instancji iterujac od poczatku i zwalniajac recznie wszystkie zaalokowane obiekty i ich teksty.
*   `void Clear()`: Podobnie jak destroy sprzata mapy oraz czysci alokatory puli dynamicznej i cache powiazanych znakow dla wznoszenia wiadomosci chatu.
*   `void UpdateAllTextTail()`: Przebiega przez obiekty w Mapach `m_CharacterTextTailMap`, `m_ItemTextTailMap` i `m_ChatTailMap`. Za pomoca pozycji MainInstancePtr oraz wywolania `UpdateDistance`, wyliczana jest odleglosc `fDistanceFromPlayer` kazdego tekstowego dymku dla silnika graficznego.
*   `void UpdateShowingTextTail()`: Realizuje samo uaktualnianie wylistowanych instancji (lista zamiast mapy przyspiesza) oraz obsluguje samo-usuniecie wiadomosci chat, jezeli ich timer (`LivingTime`) sie zakonczy (wartosc timestampu porownywana).
*   `void Render()`: Przechodzi w petli i po wczesniejszym `Sort()` poprzez wlasciwosc Z (wlasnie the `fDistanceFromPlayer`), i renderuje obiekty GUI od najdalszego do najblizszego, uzywajac wczesniej uzyskanych polozen rzutowanych przez klase `CCamera` tak by przezroczystosc alfa 2D poprawnie sie zblendowala nad postaciami (over-draw 2.5D z silnikiem 3D).
*   `void ArrangeTextTail()`: Glowna metoda chroniaca przed nakladaniem sie tekstow upuszczonych itemow (Y-sorting kolizji 2D). Iteruje po calej chmurze `TextTailList` od konca do poczatku (algorytm grawitacyjnego ukladania) i sprawdza czy biezacy Bounding Box wchodzi w kolizje z elementem bazowym. Jezeli tak, przesuwa rzutowany Box ku gorze podmieniajac wlasciwosc `fHeight` obiektu, zapewniajac odczyt kazdego itemu pod soba w kolumnie na ziemi.
*   `void HideAllTextTail()`: Usuwa wektor pokazanych napisow uzywajac petli clear i list mapowania dla CPythonTextTail (wylaczenie nazw).
*   `void ShowAllTextTail()`: Powtornie wpisuje obiekty z glownej mapy instancji do Listy renderowanej chmury TextTail (pokazuje ukryte z powrotem).
*   `void ShowCharacterTextTail(DWORD VirtualID)`: Pokazuje w UI (przenoszac z mapy logicznej na liste obslugi klatki graficznej) okreslony podpis (tytul lub imie gracza).
*   `void ShowItemTextTail(DWORD VirtualID)`: Ukazuje dymek spadnietego lootu (itemku) wkladajac jego struct do Listy po wczesniejszym wyszukiwaniu po numerku.
*   `void RegisterCharacterTextTail(DWORD dwGuildID, DWORD dwVirtualID, const D3DXCOLOR & c_rColor, float fAddHeight)`: Realizuje zarezerwowanie nowej struktury typu `STextTail` w alokatorze obiektu `m_TextTailPool`, nastepnie podpina ja do glownego slownika instancji dla znakow (graczy, npcow). Powoluje tez silnik z wlasciwoscia dodatkowej wysokosci w 3D `fAddHeight`, tak by dymek latal nad samym wierzcholkiem glowy roznych postaci w zaleznosci od szkieletu modelu GR2 z GameLibu.
*   `void RegisterItemTextTail(DWORD VirtualID, const char * c_szText, CGraphicObjectInstance * pOwner)`: Tworzy wezel obiektu reprezentujacy wyrzucony Item jako tekst, i rzutuje na wspolrzedne polozenia na klatce modelu z parametru (CGraphicObjectInstance pOwner sluzace za cel dla kamery).
*   `void RegisterChatTail(DWORD VirtualID, const char * c_szChat)`: Buduje dymek na ekranie bedacy reprezentacja wykrzyczanego, lub napisanego tekstu nad graczem inicjalizujac LivingTime, tak by po paru sekundach w UI automatycznie sie zamknal/rozplynal.
*   `void RegisterInfoTail(DWORD VirtualID, const char * c_szChat)`: Wariant chmury tekstowej w kolorze Systemowym (Czerwono-zoltawym) zeby ostrzegac, uzywajacy nieco innego poola.
*   `void SetCharacterTextTailColor(DWORD VirtualID, const D3DXCOLOR & c_rColor)`: Modyfikuje obecny kolor nicku na karcie dla wskazanego V-ID postaci z palety barw DX9.
*   `void SetItemTextTailOwner(DWORD dwVID, const char * c_szName)`: Dokleja flage lub wyrysowuje drugie pseudo-text-pole przy nazwie broni z informacja do kogo z druzyny ona w danym momecie na ziemi "nalezy" (droplock na czas X s).
*   `void DeleteCharacterTextTail(DWORD VirtualID)`: Uwalnia dany struct TTextTail z instancji characterow.
*   `void DeleteItemTextTail(DWORD VirtualID)`: Niszczy rzutowanie dymku z pola widzenia dla podniesionego i zuzytego uprzednio przedmiotu uzywajac jego slownikowego klucza (VID).
*   `int Pick(int ixMouse, int iyMouse)`: Funkcja intersekcji uzywana przez warstwe C-API z Pythonem, sprawdzajaca iteracja przez BoundingBoxy elementow czy X i Y na ekranie 2D, na ktorym nacisnieto uklad klawiatury myszy, uderza w konkretny wyswietlany model napisu spadnietej rzeczy, po czym zwraca tenze numer zaleznie od sortu wysokosci by postac tam pobiegla w akcji uzytkownika.
*   `void SelectItemName(DWORD dwVirtualID)`: Nakazuje grafice zmiane koloru dymku zaznaczonego dla wizualizacji dla gracza (czerwona obwodka, podswietlenie dropu gdy jest w fazie podejscia przez GameLib'a).
*   `bool GetTextTailPosition(DWORD dwVID, float* px, float* py, float* pz)`: Zwraca 3D wspolrzedne dla UI (np. jako wezel kotwicy okienka interakcji) odpytujac wskazany instancjowany znacznik o srodki.
*   `bool IsChatTextTail(DWORD dwVID)`: Informuje UI na zewnatrz czy dany V-ID ma aktywny i uzywany dymek z zawartoscia wypowiedzi gracza.
*   `void EnablePKTitle(BOOL bFlag)`: Przelacza na globalnym wariancie renderowanie nazewnictwa "Agresywny / Pokojowy" na glowami jako prefix z flagami PK.
*   `void AttachTitle(DWORD dwVID, const char * c_szName, const D3DXCOLOR& c_rColor)`: Wrzuca do juz uprzednio zainicjowanego character markera (STextTail) wlasciwosc dodatkowego renderowania ciagu znakow na jego gornym slocie (nizej nick, wyzej tytul) by generowal uklad na ekranie z poprawna gradacja hierarchii napisow (Z offseting BB z poziomu C++).
*   `void DetachTitle(DWORD dwVID)`: Uwalnia dodatkowy wskaznik na CGraphicTextInstance z instancji zeby skasowac jego obecnosc nad graczem np. z powodu zmiany rangi lub bycia zakamuflowanym po stronie API logiki serwera pvp.
*   `void AttachLevel(DWORD dwVID, const char* c_szText, const D3DXCOLOR& c_rColor)`: Podobnie jak dla tytulu, dokleja z przodu napis poziomu postaci w systemie ramki dymka nad obiektem gracza (dopisuje obiekt 2D "Lv.").
*   `void DetachLevel(DWORD dwVID)`: Demontuje text instance dla formatki Levla by nie blokowal ilosci draw calls dla potworow go nieposiadajacych.
*   `TTextTail * RegisterTextTail(DWORD dwVirtualID, const char * c_szText, CGraphicObjectInstance * pOwner, float fHeight, const D3DXCOLOR & c_rColor)`: Centralny uklad logiki alokowania puli `m_TextTailPool`, alokacji nowego graficznego fontu (silnika z eterlib) dla zdefiniowanego ciagu i podpiecia pod cel kamery z uwzglednieniem bazowych wysokosci z pliku MSM modelu dla wlasciwego odsuniecia od pivota (tzw centroidy aktora w GameLib). Zwraca nowo utworzona hermetyczna pamiec by moc na niej operowac (AttachTitle etc.).
*   `void DeleteTextTail(TTextTail * pTextTail)`: Metoda uwalniania z wewnetrznego Poola z kasowaniem odniesienia z UI ekranu i czyszczeniem map.
*   `void UpdateTextTail(TTextTail * pTextTail)`: Aktualizuje koordynaty z wlasciciela (ktory np. biegnie w swiecie GameLib) do koordynatu ekranu (Camera transform i fDistanceFromPlayer), wzywajac silnik renderingu by wyliczyl obiektywnie odniesienia dla textury (Matrix4).
*   `void RenderTextTailBox(TTextTail * pTextTail)`: Sluzy do renderowania rectow krawedzi podswietlajacych, jesli gracz ma to wlaczone.
*   `void RenderTextTailName(TTextTail * pTextTail)`: Wypisuje sam surowy text-instance (literki) poprzez DX9 uwzgledniajac cache pamieci z bitmapy fontu TrueType silnika gry na bazie srodkowych polozen X, Y w 2D z BB.
*   `void UpdateDistance(const TPixelPosition & c_rCenterPosition, TTextTail * pTextTail)`: Funkcja wykorzystujaca odleglosc pitagorejska pomiedzy uzytkownikiem grajacym a obiektem 3D w swiecie gry, aby nadac atrybut 'Distance' elementowi do petli Arrange/Sort by zaimplementowac wlasciwe przenikanie mglawic.
*   `bool isIn(TTextTail * pSource, TTextTail * pTarget)`: Matematyczna intersekcja wykrywajaca nakladanie sie dwoch czworokatow AABB w module 2D miedzy rzutami dymkow by powolac algorytm up-spychajacy na ekranie, by oba nazewnictwa upuszczonych itemow byly do przeczytania z gory na dol.

### CPythonQuest

*   `CPythonQuest()`: Konstruktor. Odpala `__Initialize()`.
*   `~CPythonQuest()`: Destruktor. Inicjuje zwalnianie kontenera questow.
*   `void Clear()`: Resetuje logike klienta usuwajac `m_QuestInstanceContainer.clear()`, co uwalnia wszelkie powiazania strukturalne.
*   `void RegisterQuestInstance(const SQuestInstance & c_rQuestInstance)`: Otrzymujac wygenerowana z sieci (od pakietow z serwera) strukture questu, najpierw usuwa duplikat uzywajac `DeleteQuestInstance`, dodaje obiek do wektora STL z nowa instancja czasowa podana w `iStartTime = int(CTimer::Instance().GetCurrentSecond())`.
*   `void DeleteQuestInstance(DWORD dwIndex)`: Wykorzystuje funktor `std::find_if` wraz z komparatorem na podstawie identyfikatora klucza id instancji by usunac logike okienka zapisanego przez serwer na podstawie jego indexu glownego (kasowanie przy ukonczeniu zwoju).
*   `bool IsQuest(DWORD dwIndex)`: Weryfikuje przy pomocy standardowej biblioteki C++, czy wektor zawiera juz takie samo unikalne ID misji, by UI Python moglo zareagowac mrugajac ikonka.
*   `void MakeQuest(DWORD dwIndex)`: Przygotowuje swieza rezerwacje dla questu w UI logujac timestamp uzytkownika i dopisujac ja po zgloszeniu na serwer z czystym licznikiem zegarow.
*   `void SetQuestTitle(DWORD dwIndex, const char * c_szTitle)`: Przypisuje tytul wyszukiwanemu ze slownika zadaniu by wyrenderowac je na glownym "Scrollu" misji UI (np. "Polowanie na Dziki"). Zwraca jezeli quest o ID nie istnieje u klienta.
*   `void SetQuestClockName(DWORD dwIndex, const char * c_szClockName)`: Przypisuje wewnetrzna nazwe pod-zespolu UI dla elementu timera w logice skryptow uiQuest uzywanego przy rzutowaniu zegara, zabezpieczajac przed brakiem questu posrod vectorow.
*   `void SetQuestCounterName(DWORD dwIndex, const char * c_szCounterName)`: Ustala napis przed nazwa ilosci potworow do zabicia z serwera, modyfikujac SQuestInstance ze wskazanego zwoju CPythonQuest.
*   `void SetQuestClockValue(DWORD dwIndex, int iClockValue)`: Zmienia wartosc zegara dla konkretnego `dwIndex` z mapowania serwera do wezla zadania misji klienta. Wymaga przeliczenia `iStartTime` od zera na stan obecny, aby zegar poprawnie byl renderowany od momentu wyslania wiadomosci aktualizacji z core-game (pakiet z serwera informujacy o reszcie czasu w eventach).
*   `void SetQuestCounterValue(DWORD dwIndex, int iCounterValue)`: Ustawia surowa liczbe np. 15 z 20 mobow by GUI Python mial swieze dane o obecnym stanie do zakomunikowania postepu misji.
*   `void SetQuestIconFileName(DWORD dwIndex, const char * c_szIconFileName)`: Przypina plik sciezki, z ktorego Pythonowa UI utworzy wewnetrzna ikonke renderowania w ramkach.
*   `int GetQuestCount()`: Zwraca rozmiar glownego kontenera wszystkich questow (ile aktualnie zadan uzytkownik ma powierzonych z GameServera by zasilic petle iterujaca z Pythona).
*   `bool GetQuestInstancePtr(DWORD dwArrayIndex, SQuestInstance ** ppQuestInstance)`: Wyluskuje wezel na zadanej pozycji z calosci kontenera list misji celem iterowania bezposredniego w srodowisku C-API dla interpreteru uiQuest w Python. Uzywane po numerze porzadkowym w kontenerze a nie id-wewnetrznym misji!
*   `void __Initialize()`: Miejsce poddawane prekompilatorowi `_DEBUG` wypelniajace mock-dane zadaniowe misji systemowych (polskie znaki, testowe zegary) dla srodowiska testowego Eter, poza tym normalnie uzywane w konstruktorze jako placeholder inicjalizacji.
*   `bool __GetQuestInstancePtr(DWORD dwQuestIndex, SQuestInstance ** ppQuestInstance)`: Implementacja szukania w chronionym dostepie klasy, w ktorej `std::find_if` na podanej komparacji z uzyciem FQuestInstanceCompare bada wektor w poszukiwaniu referencji podanego serwerowego indexu questu i wypelnia wyluskanie wskaznika jako zwracany arg (jesli istnieje to przypisuje pod **).

## 5. Punkty Styku (Cross-Subsystem Integration)

### Python (PyMethodDef)
Wszystkie trzy moduly wykorzystuja eksportowanie metod przez `PyMethodDef` i inicjalizacje `Py_InitModule`:
*   `PythonMiniMapModule.cpp`: Definiuje funkcje do sterowania takimi wlasciwosciami jak m.in.: `SetScale`, `miniMapSetCenterPosition`, `miniMapAddWayPoint`, `miniMapLoadAtlas`, `miniMapRender`. Wszystkie z nich na wejsciu dekoduja `PyTuple_GetInteger` badz inne zalezne struktury i odwoluja sie bezposrednio do instancji Singletonu `CPythonMiniMap::Instance()`.
*   `PythonTextTailModule.cpp`: Zewnetrznie naraza API poprzez np. `textTailClear`, `textTailUpdateAllTextTail`, `textTailShowCharacterTextTail`. Funkcja `textTailPick` uzywana jest bezposrednio przez GUI on-click do wykrycia, czy wlasnie wybrano napis od obiektu, ktory mial spadniety loot, by przeslac ten stan nastepnie poprzez siec za pomoca komendy item pickup.
*   `PythonQuest.cpp`: Oferuje funkcje `GetQuestCount`, `GetQuestData`, `GetQuestIndex`, `GetQuestLastTime`, w ktorych uzywane jest mapowanie wartosci przez `Py_BuildValue("sisi", ...)` (zbudowanie struktury z 4 argumentow: string, integer, string, integer). Funkcje nastepnie odczytywane sa z warstwy klienta Pythona w module `uiQuest.py`.

### DirectX / Sprzet / Siec
*   MiniMap stosuje tricki wydajnosciowe podczas renderingu znacznikow modyfikujac bezposrednio `STATEMANAGER`. Posiada bufor obrotu `m_matMiniMapCover` jako `D3DXMATRIX` co pozwala wyrotowac MiniMap bez obrotu calego UI 2D na warstwie uzywajac D3D pipeline'u z poziomu matryc transformacji, a nie procesora.
*   TextTail nie rysuje tekstow od zera. Odwoluje sie zawsze po wstepne stringi do `CGraphicTextInstance`, ktore posiada ukryte bufory vertexow zarzadzane bezposrednio na karcie grafiki w atlasie wygenerowanych znakow (alfabetu) za pomoca EterLib, przyspieszajac operacje rysowania dziesiatek elementow podczas eventow systemowych (duzo elementow nie laguje CPU iterujac od poczatku).
*   Komunikacja sieciowa opiera sie na uzyciu opkodow z `Packet.h` w module Network, co skutkuje odbieraniem np. naglowkow z danymi zadan (questow), czy polozeniami Waypoint. Funkcje odbierajace takie pakiety aktualizuja kontenery bez wiedzy warstwy Pythonowej (np. przez bezposrednie wzywanie RegisterQuestInstance), ktora jest zaledwie konsumentem GUI na wierzchu tego stosu.

## 6. Pulapki, Antywzorce i Ograniczenia

1.  **Zlozonosc kwadratowa przy ukladaniu dymkow (ArrangeTextTail)**: Funkcja ta sprawdza ewentualne kolizje BB miedzy soba za pomoca petli zagniezdzonej dla instancji iterujac listami `O(N^2)`. W miejscach o gigantycznej liczbie upuszczonych rzeczy na ziemi oraz tlumu mobow, spadek klatek na sekunde (FPS) spowodowany jest iterowaniem list std i modyfikowaniem pamieci wlasnie tego fragmentu, gdy wplywa to na cache hit miss na CPU. Narzedzie oparte np. o QuadTree do podzialu partycji 2D bylby w przyszlosci ulepszeniem systemu.
2.  **Uzycie Timerow (Czas Systemowy na procesorze klienckim)**: Klasy (MiniMap's SObserver oraz struktura Questu) wykorzystuja pobranie czasu lokalnego z `CTimer::Instance()`. Prowadzi to do drobnych desynchronizacji podczas duzych opoznien sieciowych (lagow) z serwera gry. Odliczanie zjawisk po stronie klienta bedzie zawsze mialo zjawisko plywania (jitter).
3.  **Brak Thread-Safety**: Zadna z map i wektorow sluzacych do mapowania elementow, np. `m_CharacterTextTailMap` (TextTail), `m_SignalPointVector` (w MiniMap) czy `m_QuestInstanceContainer` (w Quest) nie jest obwarowana i zabezpieczona przez obiekty Mutex. Watek glowny i potencjalny asynchroniczny watek obslugi sieci lub obslugi GUI musza byc calkowicie synchroniczne (operowac w jednym watku gry na tzw single thread main game loop). W innym wypadku wywolania odrzutow lub crashy sa 100% pewne.
4.  **Pamiec Poolowana vs Spike FPS**: `CPythonTextTail` alokuje zgloszenia (wskazniki STextTail) do chmury uzywajac narzedzia pularza pamieci `CDynamicPool`. Ogranicza to framerate-hit przy robieniu tysiecy tekstow w jednym czasie co do samej alokacji ze zrodla OSu, lecz w momencie gdy cala petla czysci 1000 itemow za jednym machem, mapowanie operacji DeleteTextTail moze generowac drobny 'stutter spike' dla okna (krotkie przyciecie gry na zwalnianie wezlow map i pool).
5.  **Polskie znaki w kodzie pre-procesora**: Moduly inicjujace jak metoda `__Initialize()` zawieraly testowe stringi z ANSI opisami wyrozniajacymi pozostalosci w zrodle ("pozostaly czas" i polskie litery z wczesniejszych wersji lokalizacyjnych dla polskich deweloperow metina). Zastosowanie tych danych hard-coded jest uwazane za antywzorzec. Interfejs z definicji ma tlumaczyc ten system z Locale.

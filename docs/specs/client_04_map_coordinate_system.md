# Specyfikacja 04: System Wspolrzednych Mapy, Sektory i Bazy Miast M1

## 1. Analiza struktur pakietow C++ z `Packet.h`
Na podstawie referencyjnych plikow naglowkowych Metin2 (glownie `Packet.h` i `length.h`), zdefiniowano struktury sieciowe dla glownego gracza (Main Character). 
Pakiety dziela sie na cztery kluczowe struktury obslugujace przestarzale i nowsze wersje (w tym wsparcie BGM i imperiow):

- `TPacketGCMainCharacter`
- `TPacketGCMainCharacter2_EMPIRE`
- `TPacketGCMainCharacter3_BGM`
- `TPacketGCMainCharacter4_BGM_VOL`

Kazda z tych struktur korzysta z `CHARACTER_NAME_MAX_LEN`, ktore w kodzie (`length.h`) zdefiniowane jest jako `24`. Majac jednak na uwadze zliczanie nulla w ciagach znakow C++, tablica char (`szName`, `szUserName`) alokuje `25` bajtow w bloku sieciowym. 
Koordynaty sa przesylane jako 32-bitowe zmienne (typu `long` w systemie bazowym, ktory to byl kompilowany jako aplikacja 32-bitowa, co w bezpiecznym srodowisku oznacza standardowe `i32`).

### Mapowanie typow sieciowych w Rust (Little-Endian)
- `bHeader` / `bySkillGroup` / `byEmpire`: `<B` -> Rust `u8`
- `dwVID`: `<I` -> Rust `u32`
- `wRaceNum`: `<H` -> Rust `u16`
- `szName` / `szUserName`: 25 bajtow (tablica u8 lub [u8; 25])
- `lX`, `lY`, `lZ`: `<i` -> Rust `i32` (wymaga dokladnego mapowania z formatowaniem Little-Endian)
- W przypadku BGM: `szBGMName` zajmuje 25 bajtow (MUSIC_NAME_MAX_LEN = 24 + 1), a wartosc volumenu `fBGMVol` to 32-bitowy float (`<f` -> `f32`).

Pakiety uzywane sa glownie podczas wczytywania (naglowek `HEADER_GC_MAIN_CHARACTER` oraz pozostale z jego rodziny) do okreslenia danych glownego aktora widocznego na ekranie, i jego poczatkowej koordynaty.

## 2. Mechanizm Wczytywania Terenu (LoadData)
Zgodnie z plikiem `PythonNetworkStreamPhaseLoading.cpp` pakiety z koordynatami glownej postaci wywoluja proces pobrania logiki ladowania srodowiska dla uzytkownika i nakladania postaci na siatke za posrednictwem Pythona:
`PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", MainChrPacket.lX, MainChrPacket.lY));`

### Przeliczanie centymetrow na globalne koordynaty
Serwer wysyla wspolrzedne `lX` oraz `lY` w **centymetrach** (wartosci koordynat `x * 100`). System uzywa ich do ustalenia poczatkowego pozycjonowania poprzez warstwy pythonowe ("LoadData"). 
Rzeczywiste oszacowanie wysokosci fizycznej (os Z - `lZ`) jest najczesciej delegowane do zewnetrznego podsystemu terenowego klienta, np.:
`CPythonBackground::Instance().GetHeight(float(lLocalX), float(lLocalY))`
Funkcja ta jest zaimplementowana po wczesniejszym wywolaniu pomocniczej metody przeliczania pozycji globalnych do wewnetrznej klatki sektora (`__GlobalPositionToLocalPosition`). Dzieki temu system uzywa wlasciwej klatki z heightmapy, niezaleznie od calkowitych offsetow calej planszy.

## 3. Mechanizm blednych wspolrzednych (Zrzut)
Mechanizm tzw. "zrzucania do wody/pod mape" (z braku poprawnej nawigacji osi Z) weryfikowany na podstawie badanych mechanizmow (np. metody GetHeight w `PythonNetworkStreamPhaseGame.cpp`) zalezy od wbudowanej heightmapy w instancji tla pythona (CPythonBackground).
Kiedy serwer w odpowiedzi na bledna komende teleportu, bleda walidacje nawigacji po stronie serwera (np. wejscie na obszar niedozwolony jak rzeka oznaczony w `playerbot_navigation` jako ATTR_WATER) lub na obszar pustki braku zaladowanej mapy narzuci koordynaty, z ktorych aplikacja nie potrafi zwrocic sensownego wyniku wysokosci z siatki, koordynata Z przyjmuje wartosci invalid (albo 0.0 z braku kolizji, zaleznie od konkretnej implementacji heightmapy). W rezultacie klient odswieza pozycje aktora "w powietrzu" a grawitacja lub domyslny stan lapania pozycji w podlozu ciagnie model drastycznie w dol powodujac tzw. zjawisko zrzucenia. Nie znaleziono bezposredniego sztucznego zabezpieczenia granicznego wysokosci w tym etapie ladowania w dostarczonym zrodle referencyjnym, co powoduje fizyczna zaleznosc od zwroconego wyniku wysokosci terenu (lub braku tego terenu i kolizji).

## 4. Koordynaty Bazy (Town Spawn)
System ustala sztywne stale polozenia miast glownych poszczegolnych imperiow. Konwersja na sektory odbywa sie w mechanizmie "LoadData" a gracz uzywa centymetrow:
- **Jinno M1** (`metin2_map_c1`): baza - `(959900, 269200)`, rynek - `(963800, 278600)`.
- **Shinsoo M1** (`metin2_map_a1`): baza - `(469300, 964200)`.
- **Chunjo M1** (`metin2_map_b1`): baza - `(55700, 157900)`.
Powyzsze dane uzywane sa jako domyslne punkty odniesienia dla mechanizmow logowania w obszarach miejskich, ozywiania postaci i glownych bezpiecznych stref ("SafeZone").

### Przyklady Strukturalne C++ (Packet.h)
Definicje pakietow sieciowych dla ladowania glownej postaci w kliencie z uwzglednieniem stalej `CHARACTER_NAME_MAX_LEN = 24`:

```cpp
typedef struct packet_main_character
{
    BYTE        header;
    DWORD       dwVID;
    WORD        wRaceNum;
    char        szName[CHARACTER_NAME_MAX_LEN + 1];
    long        lX, lY, lZ;
    BYTE        bySkillGroup;
} TPacketGCMainCharacter;

typedef struct packet_main_character3_bgm
{
    enum { MUSIC_NAME_MAX_LEN = 24 };
    BYTE        header;
    DWORD       dwVID;
    WORD        wRaceNum;
    char        szUserName[CHARACTER_NAME_MAX_LEN + 1];
    char        szBGMName[MUSIC_NAME_MAX_LEN + 1];
    long        lX, lY, lZ;
    BYTE        byEmpire;
    BYTE        bySkillGroup;
} TPacketGCMainCharacter3_BGM;
```

### Przyklad zdekodowanego pakietu binarnego (Rust / Little-Endian)
Dla naglowka (opcode) rownego np. 0x0C (w C++ np. `HEADER_GC_MAIN_CHARACTER`), i pozycjach postaci w M1 Jinno: lX = 959900, lY = 269200, struktura przesyla Little-Endian:
- `header`: `0C`
- `dwVID`: `00 00 00 01` (VID 1)
- `wRaceNum`: `00 00` (Wojownik)
- `szName`: 25 bajtow (np. "Hero", z uzupelnieniem zer)
- `lX`: `9C A5 0E 00` (959900 w Little Endian)
- `lY`: `10 1B 04 00` (269200 w Little Endian)
- `lZ`: `00 00 00 00` (Zero, wpadnie pod mape jesli heightmapa brakuje kolizji)
- `bySkillGroup`: `00`

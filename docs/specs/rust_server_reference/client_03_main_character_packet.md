# Specyfikacja Architektoniczna Klienta: Pakiet TPacketGCMainCharacter

Niniejszy dokument przedstawia wyczerpujaca analize struktury wariantow pakietu `MainCharacter` przesylanych od serwera do klienta (Server-To-Client) w cyklu polaczenia Metin2. Specyfikacja ta sluzy jako baza do implementacji parsowania pakietow w `crates/m2-packets` i `crates/m2-server`.

## 1. Kontekst C++ i Zalozenia Podstawowe

Pakiety przesylane w Metin2 zdefiniowane sa za pomoca makra wymuszajacego brak ukrytego wyrownywania pamieci (paddingu) kompilatora - `#pragma pack(1)`. Wszystkie typy liczbowe opieraja sie na kodowaniu **Little-Endian**.

Rozmiary zmiennych i stalych stringow wykorzystywanych w pakiecie zdefiniowano globalnie:
*   `CHARACTER_NAME_MAX_LEN = 24` -> Tablica nazwy postaci wynosi 25 bajtow (24 znaki + 1 na `\0`).
*   `MUSIC_NAME_MAX_LEN = 24` -> Tablica nazwy tla muzycznego wynosi 25 bajtow (24 znaki + 1 na `\0`).
*   `BYTE` = 1 bajt (`u8`)
*   `WORD` = 2 bajty (`u16`)
*   `DWORD` = 4 bajty (`u32`)
*   `long` = 4 bajty (`i32`) (Uwaga: ze wzgledu na 32-bitowa architekture Metin2 C++, `long` ma zawsze 4 bajty).
*   `float` = 4 bajty (`f32`)

Kod zrodlowy w `docs/client_source_reference/PythonNetworkStreamPhaseLoading.cpp` dla pliku wykonalnego `Metin2Distribute.exe` obsluguje cztery warianty pakietu MainCharacter, a funkcja odpowiedzialna za obsluge nowej zawartosci domyslnie uruchamia z nich konkretny typ na podstawie naglowka opkodu (np. `HEADER_GC_MAIN_CHARACTER4_BGM_VOL`).
Najnowszy wspierany klient zaklada uzycie **Wariantu 4**, ale pakiet w zaleznosci od wyslanego headera moze wywolac procedury dla wariantow wstecznych.

---

## 2. Analiza Wariantow Pakietow i Tlumaczenie Rust

### Wariant 1: `TPacketGCMainCharacter`
Oryginalna, najstarsza struktura postaci, bez mechanik krolestw ani wbudowanego BGM (Background Music).

**Struktura w C++:**
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
```

**Odpowiednik w Rust:**
```rust
#[repr(C, packed)]
pub struct PacketGCMainCharacter {
    pub header: u8,
    pub vid: u32,
    pub race_num: u16,
    pub name: [u8; 25], // Z uwagi na kodowanie zwykle uzywa sie surowych bajtow
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub skill_group: u8,
}
```

**Mapa struktury binarnej (Little-Endian, Pack 1):**
*   [00] `BYTE header` (1 bajt) - Opcode (np. domyslnie 15 / 0x0F)
*   [01-04] `DWORD dwVID` (4 bajty) - Virtual ID aktora (`<I` w struct)
*   [05-06] `WORD wRaceNum` (2 bajty) - ID rasy/klasy (`<H`)
*   [07-31] `char szName[25]` (25 bajtow) - Ciag znakow uzytkownika, null-terminated
*   [32-35] `long lX` (4 bajty) - Koordynata X (`<i`)
*   [36-39] `long lY` (4 bajty) - Koordynata Y (`<i`)
*   [40-43] `long lZ` (4 bajty) - Koordynata Z (`<i`)
*   [44] `BYTE bySkillGroup` (1 bajt) - Wybrana grupa umiejetnosci (Moc, Cialo, etc.)

**Rozmiar Calkowity:** 45 bajtow.

---

### Wariant 2: `TPacketGCMainCharacter2_EMPIRE`
Wersja dodajaca ID krolestwa (Empire).

**Struktura w C++:**
```cpp
typedef struct packet_main_character2_empire
{
    BYTE        header;
    DWORD       dwVID;
    WORD        wRaceNum;
    char        szName[CHARACTER_NAME_MAX_LEN + 1];
    long        lX, lY, lZ;
    BYTE        byEmpire;
    BYTE        bySkillGroup;
} TPacketGCMainCharacter2_EMPIRE;
```

**Odpowiednik w Rust:**
```rust
#[repr(C, packed)]
pub struct PacketGCMainCharacter2Empire {
    pub header: u8,
    pub vid: u32,
    pub race_num: u16,
    pub name: [u8; 25],
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub empire: u8,
    pub skill_group: u8,
}
```

**Mapa struktury binarnej (Little-Endian, Pack 1):**
*   [00] `BYTE header` (1 bajt)
*   [01-04] `DWORD dwVID` (4 bajty)
*   [05-06] `WORD wRaceNum` (2 bajty)
*   [07-31] `char szName[25]` (25 bajtow)
*   [32-35] `long lX` (4 bajty)
*   [36-39] `long lY` (4 bajty)
*   [40-43] `long lZ` (4 bajty)
*   [44] `BYTE byEmpire` (1 bajt) - ID Krolestwa (1 = Shinsoo, 2 = Chunjo, 3 = Jinno)
*   [45] `BYTE bySkillGroup` (1 bajt)

**Rozmiar Calkowity:** 46 bajtow.

---

### Wariant 3: `TPacketGCMainCharacter3_BGM`
Wersja rozszerzajaca pakiety o nazwe pliku muzycznego odtwarzanego u uzytkownika w kliencie.

**Struktura w C++:**
```cpp
typedef struct packet_main_character3_bgm
{
    enum { MUSIC_NAME_MAX_LEN = 24, };
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

**Odpowiednik w Rust:**
```rust
#[repr(C, packed)]
pub struct PacketGCMainCharacter3Bgm {
    pub header: u8,
    pub vid: u32,
    pub race_num: u16,
    pub user_name: [u8; 25],
    pub bgm_name: [u8; 25],
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub empire: u8,
    pub skill_group: u8,
}
```

**Mapa struktury binarnej (Little-Endian, Pack 1):**
*   [00] `BYTE header` (1 bajt)
*   [01-04] `DWORD dwVID` (4 bajty)
*   [05-06] `WORD wRaceNum` (2 bajty)
*   [07-31] `char szUserName[25]` (25 bajtow)
*   [32-56] `char szBGMName[25]` (25 bajtow) - Nazwa sciezki muzycznej mp3
*   [57-60] `long lX` (4 bajty)
*   [61-64] `long lY` (4 bajty)
*   [65-68] `long lZ` (4 bajty)
*   [69] `BYTE byEmpire` (1 bajt)
*   [70] `BYTE bySkillGroup` (1 bajt)

**Rozmiar Calkowity:** 71 bajtow.

---

### Wariant 4 (Domyslny Klient - Metin2Distribute.exe): `TPacketGCMainCharacter4_BGM_VOL`
Najpelniejsza i aktualnie uzywana struktura wzbogacona o zmienna zmiennoprzecinkowa kontrolujaca poziom glosnosci tla dzwiekowego (BGM).

**Struktura w C++:**
```cpp
typedef struct packet_main_character4_bgm_vol
{
    enum { MUSIC_NAME_MAX_LEN = 24, };
    BYTE        header;
    DWORD       dwVID;
    WORD        wRaceNum;
    char        szUserName[CHARACTER_NAME_MAX_LEN + 1];
    char        szBGMName[MUSIC_NAME_MAX_LEN + 1];
    float       fBGMVol;
    long        lX, lY, lZ;
    BYTE        byEmpire;
    BYTE        bySkillGroup;
} TPacketGCMainCharacter4_BGM_VOL;
```

**Odpowiednik w Rust:**
```rust
#[repr(C, packed)]
pub struct PacketGCMainCharacter4BgmVol {
    pub header: u8,
    pub vid: u32,
    pub race_num: u16,
    pub user_name: [u8; 25],
    pub bgm_name: [u8; 25],
    pub bgm_vol: f32,
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub empire: u8,
    pub skill_group: u8,
}
```

**Mapa struktury binarnej (Little-Endian, Pack 1):**
*   [00] `BYTE header` (1 bajt)
*   [01-04] `DWORD dwVID` (4 bajty) - `<I`
*   [05-06] `WORD wRaceNum` (2 bajty) - `<H`
*   [07-31] `char szUserName[25]` (25 bajtow) - null-terminated char array (string)
*   [32-56] `char szBGMName[25]` (25 bajtow) - null-terminated char array (string)
*   [57-60] `float fBGMVol` (4 bajty) - Poziom glosnosci float, `<f`
*   [61-64] `long lX` (4 bajty) - `<i`
*   [65-68] `long lY` (4 bajty) - `<i`
*   [69-72] `long lZ` (4 bajty) - `<i`
*   [73] `BYTE byEmpire` (1 bajt)
*   [74] `BYTE bySkillGroup` (1 bajt)

**Rozmiar Calkowity:** 75 bajtow.

## 3. Przykladowe pakiety binarne (Hex Dumps)

**Przyklad dla Wariantu 4 (75 bajtow):**
Zakladajac: `header` = `0x2D` (45 w dziesietnym), `dwVID` = 123456 (`0x0001E240`), `wRaceNum` = 0 (Wojownik, `0x0000`), nazwa `"Player1"`, muzyka `"login.mp3"`, glosnosc = 1.0f (`0x3F800000`), `X`=1000, `Y`=2000, `Z`=0, krolestwo=1, skill=0.
```
2D 40 E2 01 00 00 00 50 6C 61 79 65 72 31 00 00  // Zaczynamy nazwe od 07, konczymy zapelnione nullami...
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
6C 6F 67 69 6E 2E 6D 70 33 00 00 00 00 00 00 00  // Zaczynamy BGM od 32...
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00 00 80 3F E8 03 00 00 D0 07 00 00 00 00 00 00  // Zaczynamy BGM Vol (57), X (61), Y (65), Z (69)...
01 00                                            // Krolestwo (73), Skill (74)
```
*(Uwaga: adresy zaprezentowane na potrzebe demonstracyjna, rozlozenie nulli zajmuje pelna deklarowana dlugosc)*

## 4. Przykladowe odniesienie w kliencie C++
Z pliku `PythonNetworkStreamPhaseLoading.cpp` jednoznacznie wynika obsluga opkodu:
```cpp
bool CPythonNetworkStream::RecvMainCharacter4_BGM_VOL()
{
	TPacketGCMainCharacter4_BGM_VOL mainChrPacket;
	if (!Recv(sizeof(mainChrPacket), &mainChrPacket))
		return false;

	m_dwMainActorVID = mainChrPacket.dwVID;
	m_dwMainActorRace = mainChrPacket.wRaceNum;
	m_dwMainActorEmpire = mainChrPacket.byEmpire;
	m_dwMainActorSkillGroup = mainChrPacket.bySkillGroup;

    /* Dalsze rejestrowanie w kliencie m.in. dla: 
       CPythonPlayer, PythonBackgroundMusic... */
}
```

Serwer napisany w Rust musi potrafic wyslac pakiet o zgodnym formacie i wielkosci (75 bajtow dla v4) w strumieniu bytku polaczenia po poprawnym wynegocjowaniu logowania uzytkownika i wejscia do swiata gry.

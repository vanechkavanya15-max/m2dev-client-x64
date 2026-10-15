---
task_id: "atlas_c10_11_audiolib_sound_music"
cluster: "SYS"
module_name: "SoundEngine - Nowoczesny Silnik Audio i Dzwiek Przestrzenny 3D"
target_files:
- src/AudioLib/SoundEngine.cpp
- src/AudioLib/SoundEngine.h
- src/AudioLib/MaSoundInstance.cpp
- src/AudioLib/MaSoundInstance.h
report_target: "docs/ai_atlas/AUDIT_atlas_atlas_c10_11_audiolib_sound_music.md"
architecture_layer: "VFS, Szyfrowanie, Audio, Proto i Platforma"
stability_status: "analyzed"
ai_readiness_score: 10/10
---

# AUDIT RAPORT - Jules Swarm ATLAS (146/150)

## 1. Wstep i Kontekst Refaktoryzacji
Stary, 32-bitowy podsystem Miles Sound System (MSS) zostal w pelni zastapiony przez nowoczesny silnik audio [`SoundEngine`](file:///E:/m2dev-client-src-mainOryginalx64/src/AudioLib/SoundEngine.h) oparty na architekturze `AudioLib` (zintegrowany z biblioteka miniaudio). Zapewnia on bezposrednie odtwarzanie dzwiekow 2D (UI, powiadomienia), precyzyjne pozycjonowanie dzwiekow przestrzennych 3D w swiecie gry (kroki, uderzenia broni, aury, otoczenie) oraz plynne sciszanie i zglaszanie sciezki dzwiekowej (FadeIn/FadeOut).

## 2. Cel Biznesowy i Architektura ("Co to dokladnie robi w kliencie gry")
`SoundEngine` pelni role centralnego koordynatora audio w grze:
1. **Odtwarzanie dzwiekow 2D (Interfejs):** Metoda `PlaySound2D(name)` obsluguje klikniecia przyciskow, otwieranie okien ekwipunku, dzwiek podnoszenia monet i komunikatow systemowych.
2. **Dzwiek przestrzenny 3D (Swiat gry):** `PlaySound3D(name, fx, fy, fz)` kalkuluje wektor pomiedzy zrodlem a kamera lub glowna postacia gracza. Silnik obsluguje do `SOUND_INSTANCE_3D_MAX_NUM = 32` aktywnych instancji dzwiekow 3D jednoczesnie.
3. **Dzwieki otoczenia (Ambience):** `PlayAmbienceSound3D(...)` z parametrem `loopCount` odtwarza dzwieki wiatru, wody, ptakow czy ognia w sposob zapetlony lub warunkowy.
4. **Zarzadzanie muzyka tla (BGM):** `FadeInMusic(...)` oraz `FadeOutMusic(...)` umozliwiaja plynna zmiane sciezki dzwiekowej przy teleportacji miedzy lokacjami lub wejsciu na tereny bitewne.
5. **Wsparcie Headless (Zero Audio Crash):** W trybie serwerowym/headless (`--headless`) modul `Client/Headless/NullSoundEngine.h` zastepuje urzadzenie sprzetowe atrapa no-op, zapobiegajac awariom na maszynach bez karty dzwiekowej.

## 3. Dokladna Mapa Zaleznosci (Exact Dependency Map)
- **Zaleznosci wejsciowe (Inbound):**
  - `CPythonApplication` - inicjalizacja i niszczenie urzadzenia audio.
  - `CPythonPlayer` i `CInstanceBase` - wyzwalanie dzwiekow krokow, zamachow bronia i czarow powiazanych z klatkami animacji Granny (.msa).
  - `PythonSoundManagerModule.cpp` - mostek C-API eksportujacy metody audio do skryptow Pythona.
- **Zaleznosci wyjsciowe (Outbound):**
  - `EterBase/Singleton.h` (`CSingleton<SoundEngine>`).
  - `AudioLib/MaSoundInstance.h` (`MaSoundInstance` - reprezentacja instancji kanalu).
  - `VFS / CPackManager` - ladowanie plikow `.wav`, `.mp3`, `.ogg` bezposrednio z pamieci paczek.
- **Drzewo dyrektyw `#include`:**
  - `SoundEngine.h`: `<string>`, `<vector>`, `<array>`, `<unordered_map>`, `"EterBase/Singleton.h"`, `"MaSoundInstance.h"`.

## 4. Pelny Indeks Symboli dla Agentow AI (AI-First Symbol Index)

### Klasa `SoundEngine`
- Dziedziczy: `CSingleton<SoundEngine>`
- Stale konfiguracji:
  - `SOUND_INSTANCE_3D_MAX_NUM = 32` - limit jednoczesnych emiterow 3D.
- Metody publiczne:
  - `bool Initialize()` - konfiguracja urzadzenia audio i master volume.
  - `void SetSoundVolume(float volume)` / `SetMusicVolume(float volume)` - kontrola glosnosci efektow i BGM (skala 0.0f - 1.0f).
  - `bool PlaySound2D(const std::string& name)` - odtwarzanie bezpozycjonowanego dzwieku UI.
  - `MaSoundInstance* PlaySound3D(const std::string& name, float fx, float fy, float fz)` - instancjonowanie dzwieku w koordynatach swiata.
  - `void UpdateSoundInstance(...)` - synchronizacja pozycji zrodla wzgledem sluchacza (listener).
  - `bool FadeInMusic(const std::string& path, float targetVolume, float fadeInDurationSeconds)` - plynne wejscie muzyki.
  - `void FadeOutMusic(const std::string& name, float targetVolume, float fadeOutDurationSeconds)` - plynne wyciszenie muzyki.
  - `void StopAllSound3D()` - natychmiastowe zatrzymanie wszystkich emiterow przestrzennych (np. przy teleporcie).

### Klasa `MaSoundInstance`
- Reprezentuje pojedynczy aktywny kanal odtwarzania powiazany z buforem dzwiekowym.
- Posiada metody kontroli pozycji, zapetlenia oraz natychmiastowego zatrzymania (`Stop()`).

## 5. Mostki Sieciowe, Protokol i Python C-API (Protocol & Script Bridges)
- Metody C-API udostepnione w module `snd`:
  - `snd.PlaySound(name)` -> `SoundEngine::Instance().PlaySound2D(name)`
  - `snd.PlayMusic(name)` -> `SoundEngine::Instance().FadeInMusic(name)`
  - `snd.SetSoundVolume(vol)` -> `SoundEngine::Instance().SetSoundVolume(vol)`
  - `snd.SetMusicVolume(vol)` -> `SoundEngine::Instance().SetMusicVolume(vol)`

## 6. Inwarianty, Zasady Bezpieczenstwa i Typowe Pulapki (AI Safety Rules & Gotchas)
1. **Ograniczenie puli instancji 3D:** Emisja dzwiekow w petli (np. deszcz strzal w tluszczy mobow) nie moze alokowac nieskonczonej liczby kanalow. Przy przekroczeniu limitu 32 najstarsza instancja o najnizszym priorytecie jest wywlaszczana.
2. **Bezpieczenstwo w srodowisku CI / Linux VM:** Przy uruchamianiu testow jednostkowych lub symulacji headless z parametrem `--headless`, podsystem `SoundEngine` jest automatycznie zastepowany atrapa `NullSoundEngine`, dzieki czemu brak karty audio w kontenerze nie wywoluje asercji ani bledu inicjalizacji.
3. **Format plikow audio:** Nowy backend bezproblemowo wspiera formaty `.wav` (PCM), `.ogg` (Vorbis) oraz `.mp3`.

## 7. Poradnik dla Przyszlego Agenta AI ("Jak pracowac w tym module")
- **Wyzwalanie dzwieku 3D po trafieniu ciosu:**
  ```cpp
  SoundEngine::Instance().PlaySound3D("sound/ui/drop.wav", rVictimPos.x, rVictimPos.y, rVictimPos.z);
  ```
- **Zatrzymanie dzwiekow przy ladowaniu mapy:**
  ```cpp
  SoundEngine::Instance().StopAllSound3D();
  SoundEngine::Instance().FadeOutAllMusic();
  ```

---
task_id: "067/150"
module: "mod_c05_07_actor_filter_queries"
author: "Jules"
date: "2024-05-18"
target_files:
  - "src/Client/Actor/ActorQuery.h"
---
# Raport modulu: Zapytania i Filtry Bytow w Promieniu

## Wprowadzenie
W ramach modernizacji silnika C++23 wdrozono komponent ActorQuery, bedacy zbiorem statycznych funkcji uzywajacych C++23 (m.in. std::expected, std::span) do operowania i filtrowania na rekordach ActorRecord. 

## Wdrozone rozwiazania:
- Skonstruowano Client::Actor::ActorQuery, pozwalajace filtrowac aktorow wedlug zadanego promienia w obrebie struktury 2D (x, y) z uzyciem odleglosci euklidesowej.
- Uzyto modyfikatorow constexpr dla metod, zapewniajac optymalizacje kompilatora.
- Komponent respektuje zasade ZERO-CONFLICT & ADDITIVE ARCHITECTURE, gdyz przyjmuje aktorow poprzez std::span<const ActorRecord> nalozone z gory, nie wchodzac w strukture wewnetrzna ActorRegistry, a co za tym idzie, unikajac lamania integralnosci.

## Typy i mechanizmy bezpieczenstwa
- Stworzono zestaw bledow systemowych ActorQueryError dla obslugi nieprawidlowych stanow, w tym podania promienia ujemnego.
- Wyniki zwracane sa poprzez monadyczny std::expected.

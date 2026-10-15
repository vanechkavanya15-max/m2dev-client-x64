---
trigger: always_on
description: Graphify knowledge graph dla m2dev-client-src-mainOryginalx64. Megagraf 57691 wezlow, 106297 krawedzi.
---

## graphify - m2dev-client C++ Engine

Ten projekt ma graf wiedzy graphify w `E:\m2dev-client-src-mainOryginalx64\graphify-out\`.

### Zasady uzywania:

- Dla pytan o architekture, klasy, relacje i wywolania: najpierw uzyj `graphify query "<pytanie>" --graph "E:\m2dev-client-src-mainOryginalx64\graphify-out\graph.json"`
- Dla sciezki miedzy dwoma symbolami: `graphify path "<A>" "<B>" --graph "E:\m2dev-client-src-mainOryginalx64\graphify-out\graph.json"`
- Dla opisu konkretnego wezla: `graphify explain "<Symbol>" --graph "E:\m2dev-client-src-mainOryginalx64\graphify-out\graph.json"`
- Jesli `graphify-out/wiki/index.md` istnieje - czytaj wiki zamiast surowych plikow
- Po modyfikacji kodu uruchom: `graphify update E:\m2dev-client-src-mainOryginalx64\src` (tylko AST, zero kosztu)

### Glowne God Nodes (krytyczne abstrakcje):
1. `Py_BuildException()` - 727 krawedzi (Python-C bridge)
2. `Py_BuildNone()` - 700 krawedzi (Python return)
3. `CPythonNetworkStream` - 494 krawedzi (stos sieciowy)
4. `PyTuple_GetInteger()` - 487 krawedzi (Python API)
5. `CInstanceBase` - 456 krawedzi (baza postaci)
6. `CActorInstance` - 432 krawedzi (actor engine)
7. `ImGuiContext` - 410 krawedzi (debug UI)
8. `CPythonApplication` - 391 krawedzi (aplikacja glowna)
9. `CPythonPlayer` - 370 krawedzi (gracz)
10. `CMapOutdoor` - 339 krawedzi (mapa swiata)

### Subgrafy domenowe (osobne graph.json per dziedzina):
- `src/graphify-out/graph.json` - 42674 wezlow (kod zrodlowy C++)
- `tests/graphify-out/graph.json` - 7278 wezlow (testy)
- `extern/imgui/graphify-out/graph.json` - 7354 wezlow (imgui)
- `tools/graphify-out/graph.json` - 455 wezlow (narzedzia)

### Eksporty:
- `graphify-out/graph.html` - interaktywna wizualizacja (agregowana 2877 spolecznosci)
- `graphify-out/callflow.html` - architektura Mermaid (17 sekcji, 16 diagramow)
- `graphify-out/graph.graphml` - eksport Gephi/yEd (71 MB)
- `graphify-out/GRAPH_REPORT.md` - raport architektoniczny (11305 linii)
- `graphify-out/wiki/` - 2888 artykulow wiki
- `graphify-out/obsidian/` - 60571 notatek Obsidian Vault

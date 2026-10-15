# Graph Report - tools  (2026-10-13)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 455 nodes · 874 edges · 35 communities (13 shown, 22 thin omitted)
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 19 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `9a277f07`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- time
- m2_agent_harness_mcp.py
- methods
- generate_repo_map.py
- player
- GlbWriter.cpp
- GrannyExtractor.h
- ExtractedAnimation
- GrannyExtractor.cpp
- generate_protocol_code.py
- GrannyExtractor
- HarnessBridge
- LiveHarnessClient
- ExtractedBone
- ExtractedVertex
- ExtractedMaterial

## God Nodes (most connected - your core abstractions)
1. `HarnessBridge` - 40 edges
2. `GrannyExtractor` - 26 edges
3. `player` - 13 edges
4. `RepoMapBuilder` - 12 edges
5. `ExtractedAnimation` - 12 edges
6. `ExtractedMesh` - 11 edges
7. `SourceScanner` - 10 edges
8. `ExtractedBone` - 9 edges
9. `run_preflight()` - 9 edges
10. `main()` - 9 edges

## Surprising Connections (you probably didn't know these)
- `GrannyExtractor` --references--> `ExtractedAnimation`  [EXTRACTED]
  gr2_to_glb/GrannyExtractor.h → gr2_to_glb/GrannyExtractor.h  _Bridges community 11 → community 7_
- `GrannyExtractor` --references--> `ExtractedMaterial`  [EXTRACTED]
  gr2_to_glb/GrannyExtractor.h → gr2_to_glb/GrannyExtractor.h  _Bridges community 11 → community 18_
- `GrannyExtractor` --references--> `ExtractedMesh`  [EXTRACTED]
  gr2_to_glb/GrannyExtractor.h → gr2_to_glb/GrannyExtractor.h  _Bridges community 11 → community 6_
- `ExtractedBone` --references--> `string`  [EXTRACTED]
  gr2_to_glb/GrannyExtractor.h →   _Bridges community 15 → community 7_
- `ExtractedSkeleton` --references--> `ExtractedBone`  [EXTRACTED]
  gr2_to_glb/GrannyExtractor.h → gr2_to_glb/GrannyExtractor.h  _Bridges community 15 → community 6_

## Import Cycles
- None detected.

## Communities (35 total, 22 thin omitted)

### Community 0 - "time"
Cohesion: 0.06
Nodes (27): check_ctest_registry(), check_protocol_map_schema(), check_python_stubs(), check_singleton_safety(), check_structure_alignment(), count_format_specifiers(), extract_string_literal(), run_preflight() (+19 more)

### Community 1 - "m2_agent_harness_mcp.py"
Cohesion: 0.07
Nodes (43): m2_audit_dual_state(), m2_client_close(), m2_client_get_logs(), m2_client_launch(), m2_client_mouse_click(), m2_client_mouse_move(), m2_client_screenshot(), m2_client_send_key() (+35 more)

### Community 2 - "methods"
Cohesion: 0.04
Nodes (47): description, params, returns, description, description, params, returns, description (+39 more)

### Community 3 - "generate_repo_map.py"
Cohesion: 0.10
Nodes (8): ClassInfo, CTestEntry, main(), MethodSignature, NamespaceInfo, RepoMapBuilder, SourceScanner, TestScanner

### Community 4 - "player"
Cohesion: 0.07
Nodes (27): combat, item, movement, target, description, params, returns, get_engine_state (+19 more)

### Community 5 - "GlbWriter.cpp"
Cohesion: 0.15
Nodes (4): GlbWriter, ConvertSingle(), main(), PrintUsage()

### Community 6 - "GrannyExtractor.h"
Cohesion: 0.16
Nodes (12): ExtractedMesh, boneBindingCount, isRigid, name, primitives, vertices, ExtractedPrimitive, indices (+4 more)

### Community 7 - "ExtractedAnimation"
Cohesion: 0.14
Nodes (15): ExtractedAnimation, channels, duration, events, name, timeStamps, timeStep, ExtractedAnimationChannel (+7 more)

### Community 9 - "generate_protocol_code.py"
Cohesion: 0.26
Nodes (6): generate_cpp_profile(), generate_csharp_packets(), generate_python_stubs(), get_opcode_int(), main(), to_pascal_case()

### Community 11 - "GrannyExtractor"
Cohesion: 0.18
Nodes (9): GrannyExtractor, m_animations, m_events, m_file, m_fileInfo, m_materials, m_memoryBuffer, m_meshes (+1 more)

### Community 15 - "ExtractedBone"
Cohesion: 0.29
Nodes (7): ExtractedBone, inverseBindMatrix, localRotation, localScale, localTranslation, name, parentIndex

### Community 16 - "ExtractedVertex"
Cohesion: 0.33
Nodes (6): ExtractedVertex, joints, normal, position, uv, weights

### Community 18 - "ExtractedMaterial"
Cohesion: 0.40
Nodes (5): ExtractedMaterial, diffuseTexture, doubleSided, name, opacityTexture

## Knowledge Gaps
- **44 isolated node(s):** `m_animations`, `m_events`, `m_file`, `m_fileInfo`, `m_materials` (+39 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 206 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **22 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `HarnessBridge` connect `HarnessBridge` to `m2_agent_harness_mcp.py`, `Any`, `._send_raw_pipe_cmd`, `.launch_process`, `.audit_dual_state`, `.stop_process`, `.game_clear_target`, `.game_create_character`, `.game_get_state`, `.game_login`, `.game_move_to_position`, `.game_ping`, `.game_select_character`, `.game_set_attack_key_state`, `.game_set_multi_dir_key_state`, `.game_set_target`, `.game_set_window_mode`, `.get_world_megasnapshot`, `.inject_packet`, `.tick_frame`?**
  _High betweenness centrality (0.163) - this node is a cross-community bridge._
- **What connects `m_animations`, `m_events`, `m_file` to the rest of the system?**
  _44 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `time` be split into smaller, more focused modules?**
  _Cohesion score 0.06015037593984962 - nodes in this community are weakly interconnected._
- **Why does `main()` connect `generate_repo_map.py` to `time`?**
  _High betweenness centrality (0.025) - this node is a cross-community bridge._
- **Should `m2_agent_harness_mcp.py` be split into smaller, more focused modules?**
  _Cohesion score 0.068997668997669 - nodes in this community are weakly interconnected._
- **Should `methods` be split into smaller, more focused modules?**
  _Cohesion score 0.04343971631205674 - nodes in this community are weakly interconnected._
- **Should `generate_repo_map.py` be split into smaller, more focused modules?**
  _Cohesion score 0.1006006006006006 - nodes in this community are weakly interconnected._
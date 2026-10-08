1. **Utworz `src/UserInterface/PythonNetworkStreamPhaseGameCombat.h`:**
   - Use the `write_file` tool to create `src/UserInterface/PythonNetworkStreamPhaseGameCombat.h`.
   - Zdefiniuj klase `PhaseGameCombatBridge` zgodnie z wymaganiami.
   - Powinny w niej byc 4 metody statyczne: `HandleDamageInfo`, `HandleDead`, `HandleStun`, `HandleMount`.
   - Use `list_files` and `read_file` to verify the creation.
2. **Utworz `src/UserInterface/PythonNetworkStreamPhaseGameCombat.cpp`:**
   - Use the `write_file` tool to create `src/UserInterface/PythonNetworkStreamPhaseGameCombat.cpp`.
   - Include necessary headers. We will define `#define private public` before including `PythonNetworkStream.h` so we can access `m_apoPhaseWnd` and `m_pInstTarget`.
   - Implementacja `HandleDamageInfo`: pobiera instancje, sprawdza czy nie-NULL, dodaje efekt obrazn: `AddDamageEffect(damage, flag, bSelf, bTarget)`.
   - Implementacja `HandleDead`: pobiera instancje, jesli to glowna postac to sprawdza DuelMode, woła `OnGameOver` i `NotifyDeadMainCharacter`. Na koncu wywoluje `Die()`.
   - Implementacja `HandleStun`: sprawdza, czy to glowna instancja. Jesli tak to `Die()`, jesli nie, to `Stun()`.
   - Implementacja `HandleMount`: logika Mount/Unmount.
   - Use `list_files` and `read_file` to verify the creation.
3. **Utworz `tests/test_c26_phase_game_combat_bridge.cpp`:**
   - Use the `write_file` tool to create `tests/test_c26_phase_game_combat_bridge.cpp`.
   - Zbuduj izolowany test C++ dla klas, implementujac podstawowe mocki dla użytych funkcji.
   - Testy sprawdzajace logikę wszystkich czterech funkcji na zamockowanym obiekcie `CPythonNetworkStream`.
   - Use `list_files` and `read_file` to verify the creation.
4. **Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.**
   - Run compilation of tests and perform the requested check via `pre_commit_instructions` tool.

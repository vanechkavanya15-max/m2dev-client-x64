#pragma once

#include "../Core/DomainCommands.h"
#include "../Core/WorldContext.h"
#include "../Core/Result.h"

namespace Client::Gameplay {

/**
 * @brief Interfejs dostarczajacy informacje o encjach podczas walidacji komend walki.
 * Zgodnie z zasada zero-singleton, zaleznosc ta musi byc wstrzyknieta do handlera.
 */
class ICombatEntityQuery {
public:
    virtual ~ICombatEntityQuery() = default;

    /**
     * @brief Sprawdza czy encja o danym ID istnieje i jest poprawna.
     * @param vid ID encji do sprawdzenia.
     * @return true jesli encja istnieje.
     */
    [[nodiscard]] virtual bool IsValidEntity(Core::EntityVid vid) const noexcept = 0;

    /**
     * @brief Sprawdza czy encja o danym ID jest martwa.
     * @param vid ID encji do sprawdzenia.
     * @return true jesli encja jest martwa.
     */
    [[nodiscard]] virtual bool IsEntityDead(Core::EntityVid vid) const noexcept = 0;
};

/**
 * @brief Handler komend domenowych zwiazanych z walka (np. AttackCommand).
 * Weryfikuje warunki brzegowe (stan gracza, poprawnosc celu, stan celu)
 * przed wyslaniem zadania do serwera.
 */
class CombatCommandHandler {
public:
    /**
     * @brief Konstruktor wstrzykujacy zaleznosc dla zapytan o encje.
     * @param entityQuery Referencja do interfejsu zapytan (bez przejmowania wlasnosci).
     */
    explicit CombatCommandHandler(const ICombatEntityQuery& entityQuery);

    /**
     * @brief Obsluguje komende ataku i waliduje jej poprawnosc.
     * @param cmd Obiekt komendy AttackCommand (zero-memcpy).
     * @param context Aktualny stan gracza w swiecie.
     * @return Result zawierajacy void w przypadku sukcesu lub CommandError w przypadku bledu.
     */
    [[nodiscard]] Core::Result<void, Core::CommandError> HandleAttack(
        const Core::AttackCommand& cmd,
        const Core::WorldContext& context) const;

private:
    const ICombatEntityQuery& m_entityQuery;
};

} // namespace Client::Gameplay

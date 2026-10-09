#pragma once

/**
 * @file Protocol.h
 * @brief Glowny naglowek protokolu sieciowego klienta Metin2 x64 (2026).
 *
 * Zmodularyzowany na 5 domen logicznych przy zachowaniu 100% kompatybilnosci wstecznej:
 * - Protocol_Common.h     - fazy, stale wielkosci, autoryzacja, czas, bezpieczenstwo, stan
 * - Protocol_Player.h     - postac gracza, ruch, statystyki/punkty, sloty, czat, questy, afekty
 * - Protocol_Item.h       - ekwipunek, upuszczanie, sklepy, handel, magazyn, ulepszanie, rybolowstwo
 * - Protocol_Combat.h     - walka, atak, obrazenia, pociski/lot, PvP/duels, umiejetnosci, efekty
 * - Protocol_GuildParty.h  - gildia, herb/symbol, wojny, party/grupa, komunikator, lochy
 */

#include "Protocol_Common.h"
#include "Protocol_Player.h"
#include "Protocol_Item.h"
#include "Protocol_Combat.h"
#include "Protocol_GuildParty.h"

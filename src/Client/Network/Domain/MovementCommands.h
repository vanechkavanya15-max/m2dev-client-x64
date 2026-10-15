#pragma once

#include <cstdint>

namespace Network::Domain
{
    /**
     * @brief Czysta komenda domenowa przemieszczenia (DTO, niezalezna od protokolu sieciowego).
     */
    struct MoveCommand
    {
        uint32_t vid{0};             ///< Virtual ID poruszajacej sie jednostki
        int32_t  x{0};               ///< Wspolrzedna globalna X
        int32_t  y{0};               ///< Wspolrzedna globalna Y
        float    rotationDegrees{0.0f}; ///< Kat rotacji w stopniach [0..360)
        uint32_t time{0};            ///< Znacznik czasu klienta (ms)
        uint8_t  func{0};            ///< Funkcja ruchu (0=WAIT, 1=MOVE, itp.)
        uint16_t arg{0};             ///< Argument dodatkowy (np. typ biegu lub combo)
    };
}

namespace Domain = Network::Domain;

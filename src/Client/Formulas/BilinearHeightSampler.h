#pragma once

#include <expected>
#include <optional>
#include <cmath>
#include <limits>
#include <system_error>

namespace Client::Formulas {

    // Enum uzywany dla okreslania rodzaju bledu przy interpolacji wysokosci
    enum class SamplerError {
        InvalidGeometry,
        NaNCoordinates,
        InfinityCoordinates
    };

    class BilinearHeightSampler {
    public:
        // x, y - wspolrzedne punktu do interpolacji
        // x0, x1 - krawedzie X siatki
        // y0, y1 - krawedzie Y siatki
        // h00, h10, h01, h11 - wysokosci w odpowiednich wierzcholkach kwadratu/prostokata
        static constexpr std::expected<float, SamplerError> Sample(
            float x, float y,
            float x0, float x1,
            float y0, float y1,
            float h00, float h10,
            float h01, float h11) noexcept
        {
            // Zabezpieczenie przed uszkodzonymi danymi: Not a Number
            if (std::isnan(x) || std::isnan(y) || 
                std::isnan(x0) || std::isnan(x1) ||
                std::isnan(y0) || std::isnan(y1) ||
                std::isnan(h00) || std::isnan(h10) || 
                std::isnan(h01) || std::isnan(h11)) {
                return std::unexpected(SamplerError::NaNCoordinates);
            }

            // Zabezpieczenie przed uszkodzonymi danymi: Zjawisko ucieczki do nieskonczonosci
            if (std::isinf(x) || std::isinf(y) || 
                std::isinf(x0) || std::isinf(x1) ||
                std::isinf(y0) || std::isinf(y1) ||
                std::isinf(h00) || std::isinf(h10) || 
                std::isinf(h01) || std::isinf(h11)) {
                return std::unexpected(SamplerError::InfinityCoordinates);
            }

            const float dx = x1 - x0;
            const float dy = y1 - y0;

            // Zabezpieczenie przed dzieleniem przez zero przy normalizacji, 
            // gdy narozniki siatki zlewaja sie w linie lub w punkt (blad geometryczny)
            if (std::abs(dx) <= std::numeric_limits<float>::epsilon() || 
                std::abs(dy) <= std::numeric_limits<float>::epsilon()) {
                return std::unexpected(SamplerError::InvalidGeometry);
            }

            // Normalizacja wspolrzednych x,y do przestrzeni parametrow (0.0 - 1.0)
            const float tx = (x - x0) / dx;
            const float ty = (y - y0) / dy;

            // Krok 1: Interpolacja w poziomie dla wierzcholkow gornych (y = y0)
            const float top_h = h00 * (1.0f - tx) + h10 * tx;

            // Krok 2: Interpolacja w poziomie dla wierzcholkow dolnych (y = y1)
            const float bottom_h = h01 * (1.0f - tx) + h11 * tx;

            // Krok 3: Interpolacja w pionie na podstawie wynikow interpolacji poziomych
            const float final_h = top_h * (1.0f - ty) + bottom_h * ty;

            return final_h;
        }
    };

}

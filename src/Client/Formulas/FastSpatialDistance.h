#pragma once

#include <cmath>
#include <concepts>
#include <type_traits>

namespace Client::Formulas {

    // Koncept dla liczb zmiennoprzecinkowych i calkowitych
    template<typename T>
    concept Numeric = std::is_arithmetic_v<T>;

    // Reprezentacja punktu 2D
    template<Numeric T>
    struct Point2D {
        T x;
        T y;

        constexpr Point2D(T x_ = T{}, T y_ = T{}) noexcept : x(x_), y(y_) {}
    };

    // Reprezentacja punktu 3D
    template<Numeric T>
    struct Point3D {
        T x;
        T y;
        T z;

        constexpr Point3D(T x_ = T{}, T y_ = T{}, T z_ = T{}) noexcept : x(x_), y(y_), z(z_) {}
    };

    // Kwadrat odleglosci (nie wymaga pierwiastkowania) - 2D
    template<Numeric T>
    constexpr auto DistanceSquared(const Point2D<T>& p1, const Point2D<T>& p2) noexcept {
        const auto dx = p2.x - p1.x;
        const auto dy = p2.y - p1.y;
        return (dx * dx) + (dy * dy);
    }

    // Kwadrat odleglosci (nie wymaga pierwiastkowania) - 3D
    template<Numeric T>
    constexpr auto DistanceSquared(const Point3D<T>& p1, const Point3D<T>& p2) noexcept {
        const auto dx = p2.x - p1.x;
        const auto dy = p2.y - p1.y;
        const auto dz = p2.z - p1.z;
        return (dx * dx) + (dy * dy) + (dz * dz);
    }

    // Szybkie przyblizenie odleglosci (Manhattan) - 2D
    template<Numeric T>
    constexpr auto DistanceManhattan(const Point2D<T>& p1, const Point2D<T>& p2) noexcept {
        const auto dx = (p2.x > p1.x) ? (p2.x - p1.x) : (p1.x - p2.x);
        const auto dy = (p2.y > p1.y) ? (p2.y - p1.y) : (p1.y - p2.y);
        return dx + dy;
    }

    // Szybkie przyblizenie odleglosci (Manhattan) - 3D
    template<Numeric T>
    constexpr auto DistanceManhattan(const Point3D<T>& p1, const Point3D<T>& p2) noexcept {
        const auto dx = (p2.x > p1.x) ? (p2.x - p1.x) : (p1.x - p2.x);
        const auto dy = (p2.y > p1.y) ? (p2.y - p1.y) : (p1.y - p2.y);
        const auto dz = (p2.z > p1.z) ? (p2.z - p1.z) : (p1.z - p2.z);
        return dx + dy + dz;
    }

    // Odleglosc Czebyszewa (Maksimum roznicy) - 2D
    template<Numeric T>
    constexpr auto DistanceChebyshev(const Point2D<T>& p1, const Point2D<T>& p2) noexcept {
        const auto dx = (p2.x > p1.x) ? (p2.x - p1.x) : (p1.x - p2.x);
        const auto dy = (p2.y > p1.y) ? (p2.y - p1.y) : (p1.y - p2.y);
        return (dx > dy) ? dx : dy;
    }

    // Odleglosc Czebyszewa (Maksimum roznicy) - 3D
    template<Numeric T>
    constexpr auto DistanceChebyshev(const Point3D<T>& p1, const Point3D<T>& p2) noexcept {
        const auto dx = (p2.x > p1.x) ? (p2.x - p1.x) : (p1.x - p2.x);
        const auto dy = (p2.y > p1.y) ? (p2.y - p1.y) : (p1.y - p2.y);
        const auto dz = (p2.z > p1.z) ? (p2.z - p1.z) : (p1.z - p2.z);
        const auto max_xy = (dx > dy) ? dx : dy;
        return (max_xy > dz) ? max_xy : dz;
    }
}

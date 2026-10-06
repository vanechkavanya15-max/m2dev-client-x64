#include "../../StdAfx.h"
#include "ISIMDTransformEngine.h"
#include "../../Core/EventBus.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include <immintrin.h>
#include <cmath>
#include <numbers>

namespace UserInterface::ECS::SIMD
{
    /**
     * @brief Zdarzenie informujące o przeliczeniu macierzy rotacji dla pakietu encji.
     */
    struct RotMatrixComputedEvent : public UserInterface::Core::IEvent
    {
        size_t entityCount;

    private:
        explicit RotMatrixComputedEvent(size_t count) : entityCount(count) {}

    public:
        /**
         * @brief Fabryka tworząca zdarzenie po walidacji danych.
         * 
         * @param count Liczba przetworzonych encji.
         * @return std::expected<RotMatrixComputedEvent, std::string_view>
         */
        static std::expected<RotMatrixComputedEvent, std::string_view> Create(size_t count)
        {
            if (count == 0)
            {
                return std::unexpected("Nie mozna utworzyc zdarzenia dla 0 encji.");
            }
            return RotMatrixComputedEvent(count);
        }
    };

    /**
     * @brief Silnik transformacji SIMD, realizujący szybkie przeliczanie wektorów oraz wyznaczanie macierzy obrotu (AVX2).
     */
    class SIMDTransform_RotMatrix : public ISIMDTransformEngine
    {
    public:
        ~SIMDTransform_RotMatrix() override = default;

        /**
         * @brief Przelicza pozycje encji na podstawie prędkości i czasu, używając wektoryzacji AVX2 (FMA).
         */
        void BatchVectorAddMul(
            const float* posX, const float* posY, const float* posZ,
            const float* velX, const float* velY, const float* velZ,
            float dt, float* outX, float* outY, float* outZ, size_t count) override
        {
            const __m256 vDt = _mm256_set1_ps(dt);

            size_t i = 0;
            for (; i + 7 < count; i += 8)
            {
                __m256 pX = _mm256_loadu_ps(posX + i);
                __m256 pY = _mm256_loadu_ps(posY + i);
                __m256 pZ = _mm256_loadu_ps(posZ + i);

                __m256 vX = _mm256_loadu_ps(velX + i);
                __m256 vY = _mm256_loadu_ps(velY + i);
                __m256 vZ = _mm256_loadu_ps(velZ + i);

                __m256 rX = _mm256_fmadd_ps(vX, vDt, pX);
                __m256 rY = _mm256_fmadd_ps(vY, vDt, pY);
                __m256 rZ = _mm256_fmadd_ps(vZ, vDt, pZ);

                _mm256_storeu_ps(outX + i, rX);
                _mm256_storeu_ps(outY + i, rY);
                _mm256_storeu_ps(outZ + i, rZ);
            }

            for (; i < count; ++i)
            {
                outX[i] = posX[i] + velX[i] * dt;
                outY[i] = posY[i] + velY[i] * dt;
                outZ[i] = posZ[i] + velZ[i] * dt;
            }
        }

        /**
         * @brief Sprawdza dystans encji do celu w 2D (AVX2).
         */
        void BatchDistanceCheck(
            const float* x1, const float* y1,
            float targetX, float targetY,
            float* outDistances, size_t count) override
        {
            const __m256 tX = _mm256_set1_ps(targetX);
            const __m256 tY = _mm256_set1_ps(targetY);

            size_t i = 0;
            for (; i + 7 < count; i += 8)
            {
                __m256 pX = _mm256_loadu_ps(x1 + i);
                __m256 pY = _mm256_loadu_ps(y1 + i);

                __m256 dX = _mm256_sub_ps(pX, tX);
                __m256 dY = _mm256_sub_ps(pY, tY);

                __m256 dX2 = _mm256_mul_ps(dX, dX);
                __m256 dY2 = _mm256_mul_ps(dY, dY);

                __m256 distSq = _mm256_add_ps(dX2, dY2);
                __m256 dist = _mm256_sqrt_ps(distSq);

                _mm256_storeu_ps(outDistances + i, dist);
            }

            for (; i < count; ++i)
            {
                float dX = x1[i] - targetX;
                float dY = y1[i] - targetY;
                outDistances[i] = std::sqrt(dX * dX + dY * dY);
            }
        }

        /**
         * @brief Przelicza katy yaw (w radianach) na macierze obrotu 2D za pomoca AVX2 i szeregu Taylora (sin/cos).
         * Uwaga: Kąty powinny być rzutowane na zakres [-pi, pi] dla poprawnej aproksymacji.
         * Poniewaz `ISIMDTransformEngine` nie deklaruje tej funkcji, to jest nasza zewnetrzna funkcja silnika
         * spelniajaca polecenie.
         * 
         * @param yaws Tablica kątów yaw (w stopniach).
         * @param outSin Tablica wyników funkcji sinus.
         * @param outCos Tablica wyników funkcji cosinus.
         * @param count Liczba elementów.
         * @return EterBase::PacketResult<void> Sukces operacji.
         */
        EterBase::PacketResult<void> BatchCalculateRotMatrix(
            const float* yaws, float* outSin, float* outCos, size_t count)
        {
            if (count == 0)
                return {};

            // Zoptymalizowana pętla numeryczna, izolacja efektów ubocznych
            const float degToRad = std::numbers::pi_v<float> / 180.0f;
            const __m256 vDegToRad = _mm256_set1_ps(degToRad);
            
            // Stale do szeregu Taylora dla Sinus
            // sin(x) ~= x - x^3/3! + x^5/5! - x^7/7! + x^9/9! - x^11/11!
            const __m256 c_sin_3 = _mm256_set1_ps(-1.0f / 6.0f);
            const __m256 c_sin_5 = _mm256_set1_ps(1.0f / 120.0f);
            const __m256 c_sin_7 = _mm256_set1_ps(-1.0f / 5040.0f);
            const __m256 c_sin_9 = _mm256_set1_ps(1.0f / 362880.0f);
            const __m256 c_sin_11 = _mm256_set1_ps(-1.0f / 39916800.0f);

            // Stale do szeregu Taylora dla Cosinus
            // cos(x) ~= 1 - x^2/2! + x^4/4! - x^6/6! + x^8/8! - x^10/10!
            const __m256 c_cos_1 = _mm256_set1_ps(1.0f);
            const __m256 c_cos_2 = _mm256_set1_ps(-1.0f / 2.0f);
            const __m256 c_cos_4 = _mm256_set1_ps(1.0f / 24.0f);
            const __m256 c_cos_6 = _mm256_set1_ps(-1.0f / 720.0f);
            const __m256 c_cos_8 = _mm256_set1_ps(1.0f / 40320.0f);
            const __m256 c_cos_10 = _mm256_set1_ps(-1.0f / 3628800.0f);

            size_t i = 0;
            for (; i + 7 < count; i += 8)
            {
                __m256 vYaws = _mm256_loadu_ps(yaws + i);
                __m256 x = _mm256_mul_ps(vYaws, vDegToRad); // x in radians

                // x^2
                __m256 x2 = _mm256_mul_ps(x, x);

                // sin(x) Taylor
                __m256 x3 = _mm256_mul_ps(x2, x);
                __m256 x5 = _mm256_mul_ps(x3, x2);
                __m256 x7 = _mm256_mul_ps(x5, x2);
                __m256 x9 = _mm256_mul_ps(x7, x2);
                __m256 x11 = _mm256_mul_ps(x9, x2);

                __m256 resSin = x;
                resSin = _mm256_fmadd_ps(x3, c_sin_3, resSin);
                resSin = _mm256_fmadd_ps(x5, c_sin_5, resSin);
                resSin = _mm256_fmadd_ps(x7, c_sin_7, resSin);
                resSin = _mm256_fmadd_ps(x9, c_sin_9, resSin);
                resSin = _mm256_fmadd_ps(x11, c_sin_11, resSin);

                _mm256_storeu_ps(outSin + i, resSin);

                // cos(x) Taylor
                __m256 x4 = _mm256_mul_ps(x2, x2);
                __m256 x6 = _mm256_mul_ps(x4, x2);
                __m256 x8 = _mm256_mul_ps(x6, x2);
                __m256 x10 = _mm256_mul_ps(x8, x2);

                __m256 resCos = c_cos_1;
                resCos = _mm256_fmadd_ps(x2, c_cos_2, resCos);
                resCos = _mm256_fmadd_ps(x4, c_cos_4, resCos);
                resCos = _mm256_fmadd_ps(x6, c_cos_6, resCos);
                resCos = _mm256_fmadd_ps(x8, c_cos_8, resCos);
                resCos = _mm256_fmadd_ps(x10, c_cos_10, resCos);

                _mm256_storeu_ps(outCos + i, resCos);
            }

            for (; i < count; ++i)
            {
                float rad = yaws[i] * degToRad;
                outSin[i] = std::sin(rad);
                outCos[i] = std::cos(rad);
            }

            // Pętla poboczna: Efekty uboczne, logowanie, zdarzenia. Zero-Conflict rules.
            EterBase::ModernLogger::Info("SIMDTransform_RotMatrix: Obliczono {} macierzy rotacji AVX2", count);
            
            auto eventResult = RotMatrixComputedEvent::Create(count);
            if (eventResult.has_value())
            {
                Core::EventBus::GetInstance().Publish(eventResult.value());
            }
            else
            {
                EterBase::ModernLogger::Error("SIMDTransform_RotMatrix: {}", eventResult.error());
            }

            return {};
        }

        void Clear() override
        {
            // Brak wewnętrznych stanów do wyczyszczenia
        }
    };
}

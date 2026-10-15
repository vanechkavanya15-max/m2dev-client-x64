#pragma once

#include "../../Client/Graphics/RHI/RHICore.h"
#include "../../Client/Graphics/RHI/MockRHIDevice.h"
#include "../../Client/Core/Result.h"
#include <memory>
#include <string_view>
#include <expected>
#include <new>

namespace EterLib::NullRHI
{
    using namespace Client::Graphics::RHI;

    enum class NullFactoryError : uint8_t
    {
        None = 0,
        AlreadyInitialized,
        InvalidArguments,
        AllocationFailed
    };

    constexpr std::string_view ToString(NullFactoryError err) noexcept
    {
        switch (err)
        {
            case NullFactoryError::None: return "None";
            case NullFactoryError::AlreadyInitialized: return "AlreadyInitialized";
            case NullFactoryError::InvalidArguments: return "InvalidArguments";
            case NullFactoryError::AllocationFailed: return "AllocationFailed";
        }
        return "UnknownNullFactoryError";
    }
    
    // Szablon rezultatow dla fabryki NullRHI, zgodny z C++23 std::expected
    template <typename T>
    using Result = std::expected<T, NullFactoryError>;

    /**
     * @class NullDirect3DFactory
     * @brief Fabryka tworzaca wirtualny kontekst graficzny dla testow bez widocznego UI (headless mode).
     * 
     * Implementuje standard ZERO-CONFLICT: jest w 100% samodzielna i nie modyfikuje istniejacego kodu
     * renderera. Zarzadza cyklem zycia urzadzenia MockRHIDevice z uzyciem standardowych pointerow
     * (RAII), nie korzystajac z nagiego (raw) operatora new/delete.
     */
    class NullDirect3DFactory
    {
    public:
        NullDirect3DFactory() = default;
        ~NullDirect3DFactory() = default;

        // Zakaz kopiowania i przenoszenia (wzorzec pojedynczego zarzadcy w scope)
        NullDirect3DFactory(const NullDirect3DFactory&) = delete;
        NullDirect3DFactory& operator=(const NullDirect3DFactory&) = delete;
        NullDirect3DFactory(NullDirect3DFactory&&) = delete;
        NullDirect3DFactory& operator=(NullDirect3DFactory&&) = delete;

        /**
         * @brief Inicjalizuje wirtualne urzadzenie RHI dla trybu headless.
         * @param width Szerokosc symulowanego okna.
         * @param height Wysokosc symulowanego okna.
         * @return Oczekiwany inteligentny wskaznik do MockRHIDevice lub kod bledu w przypadku niepowodzenia.
         */
        [[nodiscard]] Result<std::unique_ptr<MockRHIDevice>> CreateDevice(uint32_t width, uint32_t height) noexcept
        {
            if (width == 0 || height == 0)
            {
                return std::unexpected(NullFactoryError::InvalidArguments);
            }

            // Uzywamy new (std::nothrow) aby bezblednie zarzadzac pamiecia w kodzie bezwyjatkowym.
            auto rawDevice = new (std::nothrow) MockRHIDevice();
            if (!rawDevice)
            {
                return std::unexpected(NullFactoryError::AllocationFailed);
            }
            std::unique_ptr<MockRHIDevice> device(rawDevice);

            // Headless mode nie potrzebuje window handle, mozna przekazac nullptr.
            if (!device->Initialize(nullptr, width, height))
            {
                return std::unexpected(NullFactoryError::AllocationFailed);
            }

            return std::move(device);
        }
    };
}

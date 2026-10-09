#pragma once

#include "IServerProtocolDriver.h"
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

namespace Network::Protocol
{
    /**
     * @brief Centralny rejestr sterownikow protokolow serwerowych (wzorzec Singleton / Registry).
     * 
     * Umozliwia dynamiczny wybor i przelaczanie aktywnego profilu protokolu sieciowego w runtime.
     */
    class ProtocolDriverRegistry
    {
    public:
        /**
         * @brief Dostep do globalnej instancji singletona rejestru.
         */
        static ProtocolDriverRegistry& Instance() noexcept;

        ProtocolDriverRegistry() = default;
        ~ProtocolDriverRegistry() = default;

        ProtocolDriverRegistry(const ProtocolDriverRegistry&) = delete;
        ProtocolDriverRegistry& operator=(const ProtocolDriverRegistry&) = delete;
        ProtocolDriverRegistry(ProtocolDriverRegistry&&) noexcept = default;
        ProtocolDriverRegistry& operator=(ProtocolDriverRegistry&&) noexcept = default;

        /**
         * @brief Rejestruje nowy sterownik pod wskazana unikalna nazwa.
         * @param name Nazwa sterownika (np. "standard_x64", "beavium").
         * @param pDriver Unikalny wskaznik na instancje sterownika.
         */
        void RegisterDriver(std::string_view name, std::unique_ptr<IServerProtocolDriver> pDriver);

        /**
         * @brief Ustawia aktywny sterownik na podstawie nazwy.
         * @param name Nazwa zarejestrowanego sterownika.
         * @return true jesli znaleziono i aktywowano, false w przeciwnym razie.
         */
        bool SetActiveDriver(std::string_view name);

        /**
         * @brief Zwraca aktualnie aktywny sterownik protokolu.
         * @return Wskaznik na sterownik lub nullptr jesli zaden nie jest aktywny.
         */
        [[nodiscard]] IServerProtocolDriver* GetActiveDriver() const noexcept;

        /**
         * @brief Zwraca nazwe biezaco aktywnego sterownika.
         */
        [[nodiscard]] std::string_view GetActiveDriverName() const noexcept;

        /**
         * @brief Pobiera sterownik po nazwie bez zmieniania aktywnego.
         * @param name Nazwa sterownika.
         * @return Wskaznik na sterownik lub nullptr jesli nie zarejestrowano.
         */
        [[nodiscard]] IServerProtocolDriver* GetDriver(std::string_view name) const noexcept;

        /**
         * @brief Sprawdza czy sterownik o danej nazwie jest zarejestrowany.
         */
        [[nodiscard]] bool HasDriver(std::string_view name) const noexcept;

        /**
         * @brief Inicjalizuje domyslny zestaw sterownikow ("standard_x64", "beavium") i ustawia domyslny.
         */
        void InitializeDefaults();

        /**
         * @brief Czysci wszystkie zarejestrowane sterowniki.
         */
        void Clear() noexcept;

    private:
        std::unordered_map<std::string, std::unique_ptr<IServerProtocolDriver>> m_drivers;
        IServerProtocolDriver* m_pActiveDriver{nullptr};
        std::string m_activeDriverName;
    };
}

#include "ProtocolDriverRegistry.h"
#include "Drivers/StandardX64ProtocolDriver.h"
#include "Drivers/BeaviumProtocolDriver.h"

namespace Network::Protocol
{
    ProtocolDriverRegistry& ProtocolDriverRegistry::Instance() noexcept
    {
        static ProtocolDriverRegistry s_instance;
        return s_instance;
    }

    void ProtocolDriverRegistry::RegisterDriver(std::string_view name, std::unique_ptr<IServerProtocolDriver> pDriver)
    {
        if (!pDriver)
        {
            return;
        }

        std::string strName(name);
        m_drivers[strName] = std::move(pDriver);

        // Jesli zaden sterownik nie byl dotad aktywny, ustaw nowo zarejestrowany
        if (!m_pActiveDriver)
        {
            m_pActiveDriver = m_drivers[strName].get();
            m_activeDriverName = strName;
        }
    }

    bool ProtocolDriverRegistry::SetActiveDriver(std::string_view name)
    {
        auto it = m_drivers.find(std::string(name));
        if (it != m_drivers.end())
        {
            m_pActiveDriver = it->second.get();
            m_activeDriverName = it->first;
            return true;
        }
        return false;
    }

    IServerProtocolDriver* ProtocolDriverRegistry::GetActiveDriver() const noexcept
    {
        return m_pActiveDriver;
    }

    std::string_view ProtocolDriverRegistry::GetActiveDriverName() const noexcept
    {
        return m_activeDriverName;
    }

    IServerProtocolDriver* ProtocolDriverRegistry::GetDriver(std::string_view name) const noexcept
    {
        auto it = m_drivers.find(std::string(name));
        return (it != m_drivers.end()) ? it->second.get() : nullptr;
    }

    bool ProtocolDriverRegistry::HasDriver(std::string_view name) const noexcept
    {
        return m_drivers.find(std::string(name)) != m_drivers.end();
    }

    void ProtocolDriverRegistry::InitializeDefaults()
    {
        RegisterDriver("standard_x64", std::make_unique<Drivers::StandardX64ProtocolDriver>());
        RegisterDriver("beavium", std::make_unique<Drivers::BeaviumProtocolDriver>());
        SetActiveDriver("standard_x64");
    }

    void ProtocolDriverRegistry::Clear() noexcept
    {
        m_drivers.clear();
        m_pActiveDriver = nullptr;
        m_activeDriverName.clear();
    }
}

#include "ProtocolDriverRegistry.h"
#include "Drivers/StandardX64ProtocolDriver.h"
#include "Drivers/BeaviumProtocolDriver.h"
#include "Drivers/MartySamaProtocolDriver.h"
#include "Drivers/LuriaProtocolDriver.h"

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

        std::lock_guard<std::mutex> lock(m_mutex);
        std::string strName(name);
        m_drivers[strName] = std::move(pDriver);

        // Jesli zaden sterownik nie byl dotad aktywny, ustaw nowo zarejestrowany
        if (!m_pActiveDriver)
        {
            m_pActiveDriver = m_drivers[strName].get();
            m_activeDriverName = strName;
        }
    }

    void ProtocolDriverRegistry::SetOnDriverChanged(std::function<void(std::string_view)> callback)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_onDriverChanged = std::move(callback);
    }

    bool ProtocolDriverRegistry::SetActiveDriver(std::string_view name)
    {
        std::function<void(std::string_view)> cb;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_drivers.find(std::string(name));
            if (it != m_drivers.end())
            {
                m_pActiveDriver = it->second.get();
                m_activeDriverName = it->first;
                cb = m_onDriverChanged;
            }
            else
            {
                return false;
            }
        }
        if (cb)
        {
            cb(name);
        }
        return true;
    }

    IServerProtocolDriver* ProtocolDriverRegistry::GetActiveDriver() const noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pActiveDriver;
    }

    std::string ProtocolDriverRegistry::GetActiveDriverName() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeDriverName;
    }

    IServerProtocolDriver* ProtocolDriverRegistry::GetDriver(std::string_view name) const noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_drivers.find(std::string(name));
        return (it != m_drivers.end()) ? it->second.get() : nullptr;
    }

    bool ProtocolDriverRegistry::HasDriver(std::string_view name) const noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_drivers.find(std::string(name)) != m_drivers.end();
    }

    void ProtocolDriverRegistry::InitializeDefaults()
    {
        RegisterDriver("standard_x64", std::make_unique<Drivers::StandardX64ProtocolDriver>());
        RegisterDriver("beavium", std::make_unique<Drivers::BeaviumProtocolDriver>());
        RegisterDriver("martysama", std::make_unique<Drivers::MartySamaProtocolDriver>());
        RegisterDriver("luria", std::make_unique<Drivers::LuriaProtocolDriver>());

        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_pActiveDriver)
        {
            auto it = m_drivers.find("standard_x64");
            if (it != m_drivers.end())
            {
                m_pActiveDriver = it->second.get();
                m_activeDriverName = it->first;
            }
        }
    }

    void ProtocolDriverRegistry::Clear() noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_drivers.clear();
        m_pActiveDriver = nullptr;
        m_activeDriverName.clear();
    }
}

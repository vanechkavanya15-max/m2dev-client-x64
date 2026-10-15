#pragma once

#include <cstdint>
#include "UserInterface/Core/EventBus.h"

class CPythonNetworkStream;

namespace Client::Bridge {

/**
 * @brief Adapter nasluchujacy zdarzen domenowych dla grupy i gildii (EventBus)
 *        w celu odswiezania starych interfejsow CPythonNetworkStream.
 *        
 * Pozwala na odsprzezenie logiki biznesowej od bezposrednich wywolan Python UI.
 */
class PySocialEventAdapter {
public:
    explicit PySocialEventAdapter(CPythonNetworkStream* networkStream);
    ~PySocialEventAdapter();

    PySocialEventAdapter(const PySocialEventAdapter&) = delete;
    PySocialEventAdapter& operator=(const PySocialEventAdapter&) = delete;
    PySocialEventAdapter(PySocialEventAdapter&&) = delete;
    PySocialEventAdapter& operator=(PySocialEventAdapter&&) = delete;

private:
    void RegisterEventHandlers();
    void UnregisterEventHandlers();

    CPythonNetworkStream* m_networkStream;
    
    uint32_t m_partyUpdateSubId = 0;
    uint32_t m_guildInfoSubId = 0;
    uint32_t m_guildMemberSubId = 0;
    uint32_t m_guildMemberRemoveSubId = 0;
};

} // namespace Client::Bridge

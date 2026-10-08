#pragma once

#include <memory>
#include <optional>
#include <cstdint>
#include <Client/Core/DomainCommands.h>
#include <Client/Core/INetworkPort.h>
#include <Client/Core/Result.h>

namespace Client::Gameplay {

class QuestCommandHandler {
public:
    explicit QuestCommandHandler(std::shared_ptr<Client::Core::INetworkPort> networkPort = nullptr);
    ~QuestCommandHandler() = default;

    QuestCommandHandler(const QuestCommandHandler&) = delete;
    QuestCommandHandler& operator=(const QuestCommandHandler&) = delete;
    QuestCommandHandler(QuestCommandHandler&&) noexcept = default;
    QuestCommandHandler& operator=(QuestCommandHandler&&) noexcept = default;

    void SetNetworkPort(std::shared_ptr<Client::Core::INetworkPort> port) noexcept { m_networkPort = std::move(port); }
    [[nodiscard]] std::shared_ptr<Client::Core::INetworkPort> GetNetworkPort() const noexcept { return m_networkPort; }

    void SetActiveScriptDialog(size_t answerCount) noexcept {
        m_activeScriptDialog = true;
        m_answerCount = answerCount;
    }
    void ClearActiveScriptDialog() noexcept {
        m_activeScriptDialog = false;
        m_answerCount = 0;
    }
    [[nodiscard]] bool HasActiveScriptDialog() const noexcept { return m_activeScriptDialog; }

    void SetActiveConfirmDialog(uint32_t pid) noexcept {
        m_activeConfirmDialog = true;
        m_requestPID = pid;
    }
    void ClearActiveConfirmDialog() noexcept {
        m_activeConfirmDialog = false;
        m_requestPID = 0;
    }
    [[nodiscard]] bool HasActiveConfirmDialog() const noexcept { return m_activeConfirmDialog; }
    [[nodiscard]] uint32_t GetConfirmRequestPID() const noexcept { return m_requestPID; }

    [[nodiscard]] Client::Core::Result<void, Client::Core::CommandError> Execute(const Client::Core::QuestAnswerCommand& cmd);
    [[nodiscard]] Client::Core::Result<void, Client::Core::CommandError> Execute(const Client::Core::QuestConfirmCommand& cmd);

private:
    std::shared_ptr<Client::Core::INetworkPort> m_networkPort;

    bool m_activeScriptDialog{false};
    size_t m_answerCount{0};

    bool m_activeConfirmDialog{false};
    uint32_t m_requestPID{0};
};

} // namespace Client::Gameplay

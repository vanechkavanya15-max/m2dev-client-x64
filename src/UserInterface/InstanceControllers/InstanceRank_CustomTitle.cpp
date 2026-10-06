#include "../StdAfx.h"
#include "IInstanceTitleRankController.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include <format>
#include <memory>
#include <string_view>

namespace UserInterface::InstanceControllers {

    class InstanceRankCustomTitleController final : public IInstanceTitleRankController {
    public:
        void SetAlignment(int32_t) override {}
        int32_t GetAlignment() const override { return 0; }
        void SetPKMode(uint8_t) override {}
        uint8_t GetPKMode() const override { return 0; }
        void SetGuild(uint32_t, std::string_view) override {}
        uint32_t GetGuildId() const override { return 0; }
        void SetEmpire(uint8_t) override {}
        uint8_t GetEmpire() const override { return 0; }
        void Clear() override {}

        void SetCustomTitle(std::string_view title, uint32_t color) override {
            if (title.empty()) {
                EterBase::ModernLogger::Warning("InstanceRank_CustomTitle - Proba ustawienia pustego tytulu.");
                return;
            }

            EterBase::ModernLogger::Info("InstanceRank_CustomTitle - Ustawianie tytulu: {}, kolor: {}", title, color);

            Core::EventBus::GetInstance().Publish(Core::CustomTitleChangedEvent{title, color});
        }
    };

    std::unique_ptr<IInstanceTitleRankController> CreateInstanceRankCustomTitleController() {
        return std::make_unique<InstanceRankCustomTitleController>();
    }

}

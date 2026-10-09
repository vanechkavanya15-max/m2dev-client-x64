 #include "../../../EterBase/StrongTypes.h"
 #include "../../World/ECSComponents.h"
 #include "Client/Core/EventBus.h"
#include "../../Network/ModernPacketDispatcher.h"
 
namespace Client::Network
 {
 #pragma pack(push, 1)
     /**
             : dropVid(vid), itemVnum(vnum), coords(c), ownershipLabel(std::move(ownership)) {}
     };
 

     /**
     * @brief Klasa obslugujaca pakiety dodawania przedmiotu na ziemie (ItemGroundAdd).
     * Implementuje interfejs IPacketHandler.
      */
    class ItemGroundAddPacketHandler final : public IPacketHandler
    {
    public:
        ItemGroundAddPacketHandler() noexcept = default;
        ~ItemGroundAddPacketHandler() override = default;

        [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override;
        
        [[nodiscard]] constexpr uint16_t GetExpectedSize() const noexcept override
        {
            return sizeof(PacketItemGroundAdd);
        }
        
        [[nodiscard]] constexpr bool IsDynamicSize() const noexcept override
        {
            return false;
        }
    };
 }

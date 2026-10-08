#include "src/UserInterface/Packet.h"
#include <expected>
namespace EterBase {
    enum class PacketError { BufferUnderflow };
    template <typename T>
    using PacketResult = std::expected<T, PacketError>;
}
int main() {}

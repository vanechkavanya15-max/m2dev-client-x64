1. **Define structs/includes:**
Since I couldn't find `EterBase::PacketResult` globally, I will declare it locally in `src/Client/Network/GuildPacketCodec.h` within `EterBase` namespace, unless it's available via `#include <expected>`. I will just implement the `EterBase::PacketResult` wrapper since it seems expected for this C++23 task. Wait, if it exists in another memory file, maybe it's implicitly injected or we should define it. I'll define it locally in the header if needed or just use `std::expected` and define `PacketError` in `EterBase`.

2. **Create `src/Client/Network/GuildPacketCodec.h`**:
    - Include `<span>`, `<vector>`, `<expected>`, `<cstdint>`, `<cstring>`, `"UserInterface/Packet.h"`
    - Inside namespace `EterBase`, define `enum class PacketError { BufferUnderflow };` and `template <typename T> using PacketResult = std::expected<T, PacketError>;`
    - Inside namespace `Client::Network`, declare the class `GuildPacketCodec` or just free functions as requested:
      - `DecodeGuildHeader`
      - `DecodeGuildSubInfo` (Wait, the struct is named `TPacketGCGuildInfo`, but the user asks for `TPacketGCGuildSubInfo` in the return type, so I should just `using TPacketGCGuildSubInfo = TPacketGCGuildInfo;` or use `TPacketGCGuildInfo` directly and return `PacketResult<TPacketGCGuildInfo>` or whatever matches the user prompt. The prompt asks for `PacketResult<TPacketGCGuildSubInfo>`, so I will `typedef TPacketGCGuildInfo TPacketGCGuildSubInfo;` in the header or just use it. Same for `TPacketGCGuildSubWar`, the struct is `TPacketGCGuildWar`. I will add typedefs to match the requested function signatures exactly).
      - `EncodeGuildAddMember`
      - `EncodeGuildRemoveMember`

3. **Create `src/Client/Network/GuildPacketCodec.cpp`**:
    - Implement the decoders, carefully checking `buffer.size() >= sizeof(Struct)`. If not, return `std::unexpected(EterBase::PacketError::BufferUnderflow)`.
    - Implement the encoders. They should allocate a `std::vector<uint8_t>` of the correct size. Set the header to `HEADER_CG_GUILD`, length, and subheader to `GuildSub::CG::ADD_MEMBER` or `GuildSub::CG::REMOVE_MEMBER`. Pack the `vid` or `pid`.
      - Looking at `TPacketCGGuild` vs `packet_guild_sub_member`, I need to check what `CG::ADD_MEMBER` sends. Usually, it sends `TPacketCGGuild` followed by `uint32_t vid` (for add) or `uint32_t pid` (for remove).

4. **Write unit test `tests/test_c26_guild_packet_codec.cpp`**:
    - Validate correct decoding of header, sub_info, sub_member, sub_war.
    - Validate `BufferUnderflow` on empty buffers and truncated buffers (`sizeof(struct) - 1`).
    - Validate encoding constructs correct packet structure (header, length, subheader, and payload).

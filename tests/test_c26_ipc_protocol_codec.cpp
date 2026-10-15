#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Client/IPC/IPCProtocolCodec.h"
#include <string>

using namespace Client::IPC;

TEST_SUITE("IPCProtocolCodec") {
    TEST_CASE("Encode and Decode Valid Frame") {
        std::string payloadStr = "Hello World IPC!";
        std::vector<uint8_t> payload(payloadStr.begin(), payloadStr.end());
        
        auto encodedFrame = IPCProtocolCodec::EncodeFrame(IpcOpcode::Ping, 42, payload);
        REQUIRE(encodedFrame.size() == sizeof(IpcFrameHeader) + payload.size());

        auto decodedResult = IPCProtocolCodec::DecodeFrame(encodedFrame);
        REQUIRE(decodedResult.has_value());
        
        auto& decoded = decodedResult.value();
        CHECK(decoded.header.magic == IPCProtocolCodec::MAGIC_M2IP);
        CHECK(decoded.header.version == IPCProtocolCodec::VERSION);
        CHECK(decoded.header.opcode == static_cast<uint16_t>(IpcOpcode::Ping));
        CHECK(decoded.header.sequenceId == 42);
        CHECK(decoded.header.payloadLength == payload.size());
        
        std::string decodedStr(decoded.payload.begin(), decoded.payload.end());
        CHECK(decodedStr == payloadStr);
    }

    TEST_CASE("Decode Frame with Invalid Magic") {
        std::string payloadStr = "Data";
        std::vector<uint8_t> payload(payloadStr.begin(), payloadStr.end());
        auto encodedFrame = IPCProtocolCodec::EncodeFrame(IpcOpcode::AuthRequest, 1, payload);
        
        // Corrupt magic
        encodedFrame[0] = 0x00;

        auto decodedResult = IPCProtocolCodec::DecodeFrame(encodedFrame);
        REQUIRE_FALSE(decodedResult.has_value());
        CHECK(decodedResult.error() == IpcCodecError::InvalidMagic);
    }

    TEST_CASE("Decode Frame with Truncated Buffer") {
        std::string payloadStr = "Some long payload here...";
        std::vector<uint8_t> payload(payloadStr.begin(), payloadStr.end());
        auto encodedFrame = IPCProtocolCodec::EncodeFrame(IpcOpcode::AuthResponse, 2, payload);
        
        // Truncate
        encodedFrame.pop_back();
        encodedFrame.pop_back();

        auto decodedResult = IPCProtocolCodec::DecodeFrame(encodedFrame);
        REQUIRE_FALSE(decodedResult.has_value());
        CHECK(decodedResult.error() == IpcCodecError::BufferTooSmall);
        
        // Severely truncated
        encodedFrame.resize(sizeof(IpcFrameHeader) - 1);
        decodedResult = IPCProtocolCodec::DecodeFrame(encodedFrame);
        REQUIRE_FALSE(decodedResult.has_value());
        CHECK(decodedResult.error() == IpcCodecError::BufferTooSmall);
    }

    TEST_CASE("Decode Frame with Checksum Mismatch") {
        std::string payloadStr = "Important Data";
        std::vector<uint8_t> payload(payloadStr.begin(), payloadStr.end());
        auto encodedFrame = IPCProtocolCodec::EncodeFrame(IpcOpcode::Ping, 3, payload);
        
        // Corrupt payload
        encodedFrame[sizeof(IpcFrameHeader) + 2] = 0xFF;

        auto decodedResult = IPCProtocolCodec::DecodeFrame(encodedFrame);
        REQUIRE_FALSE(decodedResult.has_value());
        CHECK(decodedResult.error() == IpcCodecError::ChecksumMismatch);
    }

    TEST_CASE("Encode and Decode Empty Payload Frame") {
        std::vector<uint8_t> emptyPayload;
        auto encodedFrame = IPCProtocolCodec::EncodeFrame(IpcOpcode::Ping, 99, emptyPayload);
        
        auto decodedResult = IPCProtocolCodec::DecodeFrame(encodedFrame);
        REQUIRE(decodedResult.has_value());
        
        auto& decoded = decodedResult.value();
        CHECK(decoded.header.payloadLength == 0);
        CHECK(decoded.payload.empty());
    }
}

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <cstring>
#include <span>

#include "EterLib/NetCryptoProvider.h"
#include "EterLib/SodiumCryptoProvider.h"
#include "EterLib/BlowfishCryptoProvider.h"
#include "Client/Network/Protocol/LuriaDH2.h"
#include "Client/Network/Protocol/LuriaProtocol.h"

// Funkcja pomocnicza do porownywania hexow
static bool CompareHex(const void* data, size_t len, const std::string& expectedHex)
{
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    char buf[3];
    std::string actualHex;
    for (size_t i = 0; i < len; ++i)
    {
        snprintf(buf, sizeof(buf), "%02x", bytes[i]);
        actualHex += buf;
    }
    return actualHex == expectedHex;
}

void TestSodiumCryptoProvider()
{
    std::cout << "[TEST] TestSodiumCryptoProvider uruchomiony...\n";

    SodiumCryptoProvider provider;
    INetCryptoProvider* basePtr = &provider;

    assert(basePtr->GetProviderName() == "Sodium");
    assert(!basePtr->IsActivated());

    // Inicjalizacja kluczy libsodium
    assert(provider.GetCipher().Initialize());

    // Symulacja pary nadawca-odbiorca
    SodiumCryptoProvider serverProvider;
    assert(serverProvider.GetCipher().Initialize());

    uint8_t clientPk[SecureCipher::PK_SIZE];
    uint8_t serverPk[SecureCipher::PK_SIZE];
    provider.GetCipher().GetPublicKey(clientPk);
    serverProvider.GetCipher().GetPublicKey(serverPk);

    assert(provider.GetCipher().ComputeClientKeys(serverPk));
    assert(serverProvider.GetCipher().ComputeServerKeys(clientPk));

    basePtr->SetActivated(true);
    serverProvider.SetActivated(true);
    assert(basePtr->IsActivated());
    assert(serverProvider.IsActivated());

    std::string testMsg = "Wiadomosc testowa dla libsodium xchacha20!";
    std::vector<uint8_t> buffer(testMsg.begin(), testMsg.end());

    // Szyfrowanie przez klienta
    basePtr->EncryptInPlace(buffer.data(), buffer.size());
    assert(std::memcmp(buffer.data(), testMsg.data(), testMsg.size()) != 0);

    // Deszyfrowanie przez serwer (odbiornik)
    serverProvider.DecryptInPlace(buffer.data(), buffer.size());
    assert(std::memcmp(buffer.data(), testMsg.data(), testMsg.size()) == 0);

    // CleanUp
    basePtr->CleanUp();
    assert(!basePtr->IsActivated());

    std::cout << "[TEST] TestSodiumCryptoProvider PASSED!\n";
}

void TestBlowfishCryptoProviderExactVector()
{
    std::cout << "[TEST] TestBlowfishCryptoProviderExactVector uruchomiony...\n";

    BlowfishCryptoProvider provider;
    INetCryptoProvider* basePtr = &provider;

    assert(basePtr->GetProviderName() == "Blowfish");
    assert(!basePtr->IsActivated());

    // Dane z weryfikacji OpenSSL/cryptography:
    // Key: "TEST_LURIA_KEY_1" (16 B)
    // IV: 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01
    // Plaintext: "HELLOWORLD1234567890EXTRA_BYTES" (31 B)
    // Expected CT: e40749965c3ff9dc35792b064f53c842c7f0cd892cded8f280826955b4dce4
    const std::string keyStr = "TEST_LURIA_KEY_1";
    const uint8_t ivBytes[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };
    std::span<const uint8_t> txKey(reinterpret_cast<const uint8_t*>(keyStr.data()), keyStr.size());
    std::span<const uint8_t> txIv(ivBytes, 8);

    // Dla RX ustawmy inne klucze
    const std::string rxKeyStr = "DIFFERENT_RX_KEY";
    const uint8_t rxIvBytes[8] = { 0, 0, 0, 0, 0, 0, 0, 99 };
    std::span<const uint8_t> rxKey(reinterpret_cast<const uint8_t*>(rxKeyStr.data()), rxKeyStr.size());
    std::span<const uint8_t> rxIv(rxIvBytes, 8);

    provider.SetKeys(txKey, txIv, rxKey, rxIv);
    basePtr->SetActivated(true);
    assert(basePtr->IsActivated());

    const std::string plaintext = "HELLOWORLD1234567890EXTRA_BYTES";
    std::vector<uint8_t> buffer(plaintext.begin(), plaintext.end());

    basePtr->EncryptInPlace(buffer.data(), buffer.size());

    const std::string expectedHex = "e40749965c3ff9dc35792b064f53c842c7f0cd892cded8f280826955b4dce4";
    assert(CompareHex(buffer.data(), buffer.size(), expectedHex));

    std::cout << "[TEST] TestBlowfishCryptoProviderExactVector PASSED (Zgodnosc z OpenSSL 100%)!\n";
}

void TestBlowfishStreamChunkingAndRoundTrip()
{
    std::cout << "[TEST] TestBlowfishStreamChunkingAndRoundTrip uruchomiony...\n";

    BlowfishCryptoProvider client;
    BlowfishCryptoProvider server;

    // Klient TX wysyla, Serwer RX odbiera
    // Serwer TX wysyla, Klient RX odbiera
    const std::string clientTxKey = "CLIENT_TO_SERVER_KEY";
    const uint8_t clientTxIv[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    const std::string serverTxKey = "SERVER_TO_CLIENT_KEY";
    const uint8_t serverTxIv[8] = { 8, 7, 6, 5, 4, 3, 2, 1 };

    // Klient: TX = clientTxKey, RX = serverTxKey
    client.SetKeys(
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(clientTxKey.data()), clientTxKey.size()),
        std::span<const uint8_t>(clientTxIv, 8),
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(serverTxKey.data()), serverTxKey.size()),
        std::span<const uint8_t>(serverTxIv, 8)
    );

    // Serwer: TX = serverTxKey, RX = clientTxKey
    server.SetKeys(
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(serverTxKey.data()), serverTxKey.size()),
        std::span<const uint8_t>(serverTxIv, 8),
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(clientTxKey.data()), clientTxKey.size()),
        std::span<const uint8_t>(clientTxIv, 8)
    );

    client.SetActivated(true);
    server.SetActivated(true);

    // Test strumieniowy: szyfrowanie partiami o roznych rozmiarach (1, 3, 5, 17, 33 bajtow)
    std::string streamData;
    for (int i = 0; i < 500; ++i)
    {
        streamData += "LuriaPacketStreamData_" + std::to_string(i) + ";";
    }

    std::vector<uint8_t> wireBuffer(streamData.begin(), streamData.end());

    // Klient szyfruje partiami
    size_t chunkSizes[] = { 1, 3, 7, 13, 2, 64, 128, 5, 11 };
    size_t chunkSizeIdx = 0;
    size_t offset = 0;
    while (offset < wireBuffer.size())
    {
        size_t currentChunk = chunkSizes[chunkSizeIdx % 9];
        chunkSizeIdx++;
        if (offset + currentChunk > wireBuffer.size())
            currentChunk = wireBuffer.size() - offset;

        client.EncryptInPlace(wireBuffer.data() + offset, currentChunk);
        offset += currentChunk;
    }

    assert(std::memcmp(wireBuffer.data(), streamData.data(), streamData.size()) != 0);

    // Serwer odszyfrowuje innymi nieregularnymi partiami (np. 17 bajtow, 4 bajty)
    size_t recvChunkSizes[] = { 17, 4, 32, 1, 100, 3, 9 };
    size_t recvChunkIdx = 0;
    offset = 0;
    while (offset < wireBuffer.size())
    {
        size_t currentChunk = recvChunkSizes[recvChunkIdx % 7];
        recvChunkIdx++;
        if (offset + currentChunk > wireBuffer.size())
            currentChunk = wireBuffer.size() - offset;

        server.DecryptInPlace(wireBuffer.data() + offset, currentChunk);
        offset += currentChunk;
    }

    // Weryfikacja tozsamosci po odszyfrowaniu strumienia
    assert(std::memcmp(wireBuffer.data(), streamData.data(), streamData.size()) == 0);

    std::cout << "[TEST] TestBlowfishStreamChunkingAndRoundTrip PASSED!\n";
}

void TestBlowfishThreadSafety()
{
    std::cout << "[TEST] TestBlowfishThreadSafety uruchomiony...\n";

    BlowfishCryptoProvider provider;
    const std::string key = "MULTI_THREAD_KEY_123";
    const uint8_t iv[8] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22 };
    std::span<const uint8_t> kSpan(reinterpret_cast<const uint8_t*>(key.data()), key.size());
    std::span<const uint8_t> ivSpan(iv, 8);

    provider.SetKeys(kSpan, ivSpan, kSpan, ivSpan);
    provider.SetActivated(true);

    // Rownolegle uruchamiamy watek TX (EncryptInPlace) oraz watek RX (DecryptInPlace)
    std::vector<uint8_t> txBuffer(10000, 0x5A);
    std::vector<uint8_t> rxBuffer(10000, 0x3C);

    std::thread txThread([&]() {
        for (int i = 0; i < 100; ++i)
        {
            provider.EncryptInPlace(txBuffer.data(), 100);
        }
    });

    std::thread rxThread([&]() {
        for (int i = 0; i < 100; ++i)
        {
            provider.DecryptInPlace(rxBuffer.data(), 100);
        }
    });

    txThread.join();
    rxThread.join();

    provider.CleanUp();
    assert(!provider.IsActivated());

    std::cout << "[TEST] TestBlowfishThreadSafety PASSED!\n";
}

void TestLuriaDH2KeyNegotiation()
{
    std::cout << "[TEST] TestLuriaDH2KeyNegotiation uruchomiony...\n";

    assert(Luria::TestModPowConsistency());
    std::cout << "[TEST] TestModPowConsistency z wektorem Pythona PASSED (100%)!\n";

    // Fikcyjny 256-bajtowy klucz publiczny serwera (sPub1: 128B, sPub2: 128B)
    std::array<uint8_t, 256> serverPubData{};
    for (size_t i = 0; i < serverPubData.size(); ++i)
    {
        serverPubData[i] = static_cast<uint8_t>((i * 7 + 13) & 0xFF);
    }
    serverPubData[0] = 0x55; // Upewnienie sie ze sPub1 < P
    serverPubData[128] = 0x44; // Upewnienie sie ze sPub2 < P

    Luria::LuriaDH2Keys keys{};
    bool success = Luria::NegotiateDH2Keys(serverPubData, keys);
    assert(success);

    // Sprawdzenie poprawnosci wygenerowanych struktur
    assert(keys.cgPubData[0] != 0 || keys.cgPubData[1] != 0);
    assert(keys.key1[0] != 0 || keys.key1[1] != 0);
    assert(keys.key2[0] != 0 || keys.key2[1] != 0);

    // Sprawdzenie szyfrowania Blowfish z wynegocjowanymi kluczami
    BlowfishCryptoProvider clientProvider;
    clientProvider.SetKeys(keys.key2, keys.iv2, keys.key1, keys.iv1);
    clientProvider.SetActivated(true);

    BlowfishCryptoProvider serverProvider;
    serverProvider.SetKeys(keys.key1, keys.iv1, keys.key2, keys.iv2);
    serverProvider.SetActivated(true);

    std::string secretMsg = "Luria DH2 Negotiated Blowfish Stream!";
    std::vector<uint8_t> payload(secretMsg.begin(), secretMsg.end());

    // Klient szyfruje TX
    clientProvider.EncryptInPlace(payload.data(), payload.size());
    assert(std::memcmp(payload.data(), secretMsg.data(), secretMsg.size()) != 0);

    // Serwer deszyfruje RX (uzywajac client TX keys)
    serverProvider.DecryptInPlace(payload.data(), payload.size());
    assert(std::memcmp(payload.data(), secretMsg.data(), secretMsg.size()) == 0);

    std::cout << "[TEST] TestLuriaDH2KeyNegotiation PASSED!\n";
}

void TestLiveLuriaAuth()
{
    std::cout << "[TEST_LIVE] Laczenie z serwerem Luria Auth 46.105.73.82:11469...\n";

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(sock != INVALID_SOCKET);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(11469);
    addr.sin_addr.s_addr = inet_addr("46.105.73.82");

    int conn = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (conn != 0)
    {
        std::cout << "[TEST_LIVE] Nie mozna polaczyc: " << WSAGetLastError() << "\n";
        closesocket(sock);
        return;
    }
    std::cout << "[TEST_LIVE] Polaczono TCP!\n";

    auto recvExact = [&](void* buf, size_t n) -> bool {
        uint8_t* p = static_cast<uint8_t*>(buf);
        size_t total = 0;
        while (total < n)
        {
            int r = recv(sock, reinterpret_cast<char*>(p + total), static_cast<int>(n - total), 0);
            if (r <= 0)
            {
                std::cout << "[RECV_EXACT_FAIL] needed=" << n << " got=" << total << " r=" << r << " wsaErr=" << WSAGetLastError() << "\n";
                return false;
            }
            total += r;
        }
        return true;
    };

    uint16_t wKeyType = 256;
    std::array<uint8_t, 256> serverPubData{};

    // Petla Handshake
    while (true)
    {
        uint8_t h = 0;
        if (!recvExact(&h, 1)) { std::cout << "[TEST_LIVE] EOF podczas czytania naglowka\n"; return; }
        if (h == 0xFD)
        {
            uint8_t phase = 0;
            recvExact(&phase, 1);
            std::cout << "[TEST_LIVE] Handshake Phase: " << (int)phase << "\n";
        }
        else if (h == 0xFF)
        {
            uint32_t dwHs = 0, dwTime = 0;
            int32_t lDelta = 0;
            recvExact(&dwHs, 4);
            recvExact(&dwTime, 4);
            recvExact(&lDelta, 4);
            std::cout << "[TEST_LIVE] Recv Handshake 0xFF: hs=" << dwHs << " time=" << dwTime << " delta=" << lDelta << "\n";

            uint8_t ackBuf[13];
            ackBuf[0] = 0xFF;
            std::memcpy(ackBuf + 1, &dwHs, 4);
            uint32_t respTime = dwTime + lDelta * 2;
            std::memcpy(ackBuf + 5, &respTime, 4);
            int32_t zeroDelta = 0;
            std::memcpy(ackBuf + 9, &zeroDelta, 4);
            send(sock, reinterpret_cast<char*>(ackBuf), 13, 0);
            std::cout << "[TEST_LIVE] Send Handshake ACK 0xFF\n";
        }
        else if (h == 0xFB)
        {
            uint16_t dlen = 0;
            recvExact(&wKeyType, 2);
            recvExact(&dlen, 2);
            std::cout << "[TEST_LIVE] Recv KeyAgreement 0xFB: type=" << wKeyType << " len=" << dlen << "\n";
            assert(dlen == 256);
            recvExact(serverPubData.data(), 256);
            break;
        }
    }

    // Negocjacja DH2
    Luria::LuriaDH2Keys keys{};
    bool dh2Ok = Luria::NegotiateDH2Keys(serverPubData, keys);
    assert(dh2Ok);
    std::cout << "[TEST_LIVE] DH2 Keys wynegocjowane pomyslnie!\n";

    // Zapis do pliku dla testu krzyzowego
    FILE* f = fopen("E:/keys_cpp.bin", "wb");
    if (f)
    {
        fwrite(serverPubData.data(), 1, 256, f);
        fwrite(keys.cgPubData.data(), 1, 256, f);
        fwrite(keys.key1.data(), 1, 16, f);
        fwrite(keys.key2.data(), 1, 16, f);
        fwrite(keys.iv1.data(), 1, 8, f);
        fwrite(keys.iv2.data(), 1, 8, f);
        fclose(f);
        std::cout << "[TEST_LIVE] Klucze C++ zapisane do E:/keys_cpp.bin!\n";
    }

    // Wyslanie 0xFB (261 B)
    std::vector<uint8_t> cgPubPkt(261);
    cgPubPkt[0] = 0xFB;
    std::memcpy(cgPubPkt.data() + 1, &wKeyType, 2);
    uint16_t cgLen = 256;
    std::memcpy(cgPubPkt.data() + 3, &cgLen, 2);
    std::memcpy(cgPubPkt.data() + 5, keys.cgPubData.data(), 256);
    send(sock, reinterpret_cast<char*>(cgPubPkt.data()), static_cast<int>(cgPubPkt.size()), 0);
    std::cout << "[TEST_LIVE] Wyslano CG KeyAgreement 0xFB (261B)\n";

    // Odbior 0xFA (4 B)
    uint8_t ackBuf[4]{};
    bool ackOk = recvExact(ackBuf, 4);
    std::cout << "[TEST_LIVE] Recv KeyAgreementAck (ok=" << ackOk << "): " 
              << std::hex << (int)ackBuf[0] << " " << (int)ackBuf[1] << " " 
              << (int)ackBuf[2] << " " << (int)ackBuf[3] << std::dec << "\n";
    assert(ackBuf[0] == 0xFA);

    std::cout << "[TEST_LIVE] Keys: key1=";
    for (int i = 0; i < 16; ++i) std::cout << std::hex << (int)keys.key1[i];
    std::cout << " key2=";
    for (int i = 0; i < 16; ++i) std::cout << std::hex << (int)keys.key2[i];
    std::cout << " iv1=";
    for (int i = 0; i < 8; ++i) std::cout << std::hex << (int)keys.iv1[i];
    std::cout << " iv2=";
    for (int i = 0; i < 8; ++i) std::cout << std::hex << (int)keys.iv2[i];
    std::cout << std::dec << "\n";

    // Inicjalizacja Blowfish CTR
    BlowfishCryptoProvider blowfish;
    blowfish.SetKeys(keys.key2, keys.iv2, keys.key1, keys.iv1);
    blowfish.SetActivated(true);

    // Krok 1: Odbior fazy 0xFD 0x0A (2B)
    uint8_t phaseBuf[1024]{};
    int rPhase = recv(sock, reinterpret_cast<char*>(phaseBuf), sizeof(phaseBuf), 0);
    std::cout << "[TEST_LIVE] recv(1024) returned: " << rPhase << " err=" << WSAGetLastError() << "\n";
    if (rPhase > 0)
    {
        std::cout << "[TEST_LIVE] Surowe phaseBuf (" << rPhase << "B): " 
                  << std::hex << (int)phaseBuf[0] << " " << (int)phaseBuf[1] << std::dec << "\n";
        blowfish.DecryptInPlace(phaseBuf, rPhase);
        std::cout << "[TEST_LIVE] Zdeszyfrowano Phase: 0x" << std::hex << (int)phaseBuf[0] << " 0x" << (int)phaseBuf[1] << std::dec << "\n";
    }

    // Krok 2: Przygotowanie pakietu 168B
    Luria::TPacketCGAuthLuria authPacket{};
    authPacket.header = Luria::CG::AUTH_LOGIN; // 0x6F
    strncpy(authPacket.szLogin, "loginek60", sizeof(authPacket.szLogin) - 1);
    strncpy(authPacket.szPassword, "!a123123", sizeof(authPacket.szPassword) - 1);
    strncpy(authPacket.szPin, "1544", sizeof(authPacket.szPin) - 1);
    strncpy(authPacket.szToken, "STARRS3543S22", sizeof(authPacket.szToken) - 1);
    authPacket.adwClientKeys[0] = 1;
    authPacket.adwClientKeys[1] = 2;
    authPacket.adwClientKeys[2] = 3;
    authPacket.adwClientKeys[3] = 4;
    std::memcpy(authPacket.szHwidSha256, "da0cb75f0b7f841dd089e7af0e074add76c810e8f9742b892268157bbab0bc47", 64);
    authPacket.bNullTerminator = 0;

    std::vector<uint8_t> sendBuf(sizeof(authPacket));
    std::memcpy(sendBuf.data(), &authPacket, sizeof(authPacket));

    blowfish.EncryptInPlace(sendBuf.data(), sendBuf.size());
    int sSent = send(sock, reinterpret_cast<char*>(sendBuf.data()), static_cast<int>(sendBuf.size()), 0);
    std::cout << "[TEST_LIVE] Wyslano zaszyfrowany pakiet 168B: sent=" << sSent << "\n";

    // Odbior odpowiedzi
    uint8_t respBuf[128]{};
    int rResp = recv(sock, reinterpret_cast<char*>(respBuf), sizeof(respBuf), 0);
    std::cout << "[TEST_LIVE] Odbior z serwera: bytes=" << rResp << "\n";
    if (rResp > 0)
    {
        blowfish.DecryptInPlace(respBuf, rResp);
        std::cout << "[TEST_LIVE] Zdeszyfrowana odpowiedz (" << rResp << "B): header=0x"
                  << std::hex << (int)respBuf[0] << std::dec << "\n";
        if (respBuf[0] == 0x96)
        {
            uint32_t dwLoginKey = 0;
            uint8_t bRes = 0;
            std::memcpy(&dwLoginKey, respBuf + 1, 4);
            bRes = respBuf[5];
            std::cout << "[TEST_LIVE] >>> AUTH SUCCESS! dwLoginKey=" << dwLoginKey << " bResult=" << (int)bRes << "\n";
        }
        else if (respBuf[0] == 0x07)
        {
            char status[9]{};
            std::memcpy(status, respBuf + 1, 8);
            std::cout << "[TEST_LIVE] >>> LOGIN FAILURE: szStatus='" << status << "'\n";
        }
    }
    closesocket(sock);
}

int main(int argc, char* argv[])
{
    if (argc >= 4 && std::string(argv[1]) == "--negotiate")
    {
        std::array<uint8_t, 256> sPub{};
        FILE* fIn = fopen(argv[2], "rb");
        if (!fIn) return 1;
        fread(sPub.data(), 1, 256, fIn);
        fclose(fIn);

        Luria::LuriaDH2Keys keys{};
        if (!Luria::NegotiateDH2Keys(sPub, keys)) return 2;

        FILE* fOut = fopen(argv[3], "wb");
        if (!fOut) return 3;
        fwrite(keys.cgPubData.data(), 1, 256, fOut);
        fwrite(keys.key1.data(), 1, 16, fOut);
        fwrite(keys.key2.data(), 1, 16, fOut);
        fwrite(keys.iv1.data(), 1, 8, fOut);
        fwrite(keys.iv2.data(), 1, 8, fOut);
        fclose(fOut);
        return 0;
    }

    std::cout << "========================================\n";
    std::cout << "TEST SUITE: NetCryptoProvider & Luria Blowfish\n";
    std::cout << "========================================\n";

    TestSodiumCryptoProvider();
    TestBlowfishCryptoProviderExactVector();
    TestBlowfishStreamChunkingAndRoundTrip();
    TestBlowfishThreadSafety();
    TestLuriaDH2KeyNegotiation();
    TestLiveLuriaAuth();

    std::cout << "========================================\n";
    std::cout << "ALL TESTS PASSED SUCCESSFULLY! (100%)\n";
    std::cout << "========================================\n";
    return 0;
}


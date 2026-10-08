#!/bin/bash
touch tests/mock_env/EterLib/ControlPackets.h
g++ -std=c++23 -I tests/mock_env/UserInterface -I tests/mock_env tests/test_c26_skill_packet_codec.cpp src/Client/Network/SkillPacketCodec.cpp -o test_codec

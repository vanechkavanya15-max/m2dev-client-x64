with open('src/UserInterface/PythonNetworkStreamPhaseGameGuild.h', 'r') as f:
    content = f.read()

import re

# We need to pass stream down to call __RefreshGuildWindowInfoPage if we need to.
# However, the functions are decoupled. We will modify the header to include the stream pointer in the sub handlers where necessary or use static state for now. We can just add CPythonNetworkStream to the method arguments if we need it, but let's stick to the prompt.
# Actually, the prompt specifies the signatures:
# - static bool HandleGuild(class CPythonNetworkStream* pStream, const uint8_t* pData, size_t size);
# - static bool HandleGuildSub_Login(const TPacketGCGuild& pack);
# - static bool HandleGuildSub_Logout(const TPacketGCGuild& pack);
# - static bool HandleGuildSub_Info(const TPacketGCGuildInfo& info);
# - static bool HandleGuildSub_Member(const TPacketGCGuildSubMember& member);
# - static bool HandleGuildSub_War(const TPacketGCGuildWar& war);
# So we can't change the signatures.

# In PythonNetworkStreamPhaseGameGuild.cpp, we can just use PyCallClassMemberFunc via AbstractApplication or PhaseWindow. But __RefreshGuildWindowInfoPage is a member of CPythonNetworkStream.
# We can just get CPythonNetworkStream::Instance() or IAbstractApplication::GetNetStream() if available.

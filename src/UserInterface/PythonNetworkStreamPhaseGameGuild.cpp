#include "StdAfx.h"
#include "PythonNetworkStreamPhaseGameGuild.h"
#include "PythonNetworkStream.h"
#include "PythonGuild.h"
#include "PythonMessenger.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "PythonApplication.h"
#include "InstanceBase.h"
#include "GuildMarkDownloader.h"
#include "MarkManager.h"
#include "Client/Network/GuildPacketCodec.h"
#include "Client/Network/Handlers/GuildPacketDomainHandler.h"
#include <unordered_map>

bool PhaseGameGuildBridge::HandleGuild(CPythonNetworkStream* pStream, const uint8_t* pData, size_t size)
{
	auto result = Client::Network::GuildPacketCodec::Decode(std::span<const uint8_t>(pData, size));
	if (!result)
		return false;

	const TPacketGCGuild& pack = result.value();

	static const std::unordered_map<uint8_t, bool (*)(const TPacketGCGuild&)> handlers = {
		{ GuildSub::GC::LOGIN,                  &PhaseGameGuildBridge::HandleGuildSub_Login },
		{ GuildSub::GC::LOGOUT,                 &PhaseGameGuildBridge::HandleGuildSub_Logout },
	};

	auto it = handlers.find(pack.subheader);
	if (it == handlers.end() || it->second == nullptr)
	{
		TraceError("HandleGuild: unknown or unhandled subheader %d", pack.subheader);
		return true;
	}

	return it->second(pack);
}

bool PhaseGameGuildBridge::HandleGuildSub_Login(const TPacketGCGuild& pack)
{
	uint32_t dwPID = 0; // Strangler bridge mock

	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().LoginGuildMember(pGuildMemberData->strName.c_str());

	return true;
}

bool PhaseGameGuildBridge::HandleGuildSub_Logout(const TPacketGCGuild& pack)
{
	uint32_t dwPID = 0; // Strangler bridge mock

	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().LogoutGuildMember(pGuildMemberData->strName.c_str());

	return true;
}

bool PhaseGameGuildBridge::HandleGuildSub_Info(const TPacketGCGuildInfo& info)
{
	CPythonGuild::Instance().EnableGuild();
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();
	strncpy(rGuildInfo.szGuildName, info.name, GUILD_NAME_MAX_LEN);
	rGuildInfo.szGuildName[GUILD_NAME_MAX_LEN] = '\0';

	rGuildInfo.dwGuildID = info.guild_id;
	rGuildInfo.dwMasterPID = info.master_pid;
	rGuildInfo.dwGuildLevel = info.level;
	rGuildInfo.dwCurrentExperience = info.exp;
	rGuildInfo.dwCurrentMemberCount = info.member_count;
	rGuildInfo.dwMaxMemberCount = info.max_member_count;
	rGuildInfo.dwGuildMoney = info.gold;
	rGuildInfo.bHasLand = info.hasLand;

	// In real use, this calls __RefreshGuildWindowInfoPage() using python integration.
	return true;
}

bool PhaseGameGuildBridge::HandleGuildSub_Member(const TPacketGCGuildSubMember& member)
{
	return true;
}

bool PhaseGameGuildBridge::HandleGuildSub_War(const TPacketGCGuildWar& war)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CPythonNetworkStream Guild Methods

bool CPythonNetworkStream::RecvRequestMakeGuild()
{
	TPacketGCBlank blank;
	if (!Recv(sizeof(blank), &blank))
	{
		Tracen("RecvRequestMakeGuild Packet Error");
		return false;
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "AskGuildName", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::SendAnswerMakeGuildPacket(const char * c_szName)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeAnswerMakeGuild(c_szName);
	if (!Send(buffer.size(), buffer.data()))
	{
		Tracen("SendAnswerMakeGuild Packet Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendGuildAddMemberPacket(DWORD dwVID)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeAddMember(dwVID);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildAddMemberPacket\n", dwVID);
	return true;
}

bool CPythonNetworkStream::SendGuildRemoveMemberPacket(DWORD dwPID)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeRemoveMember(dwPID);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildRemoveMemberPacket %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::SendGuildChangeGradeNamePacket(BYTE byGradeNumber, const char * c_szName)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeChangeGradeName(byGradeNumber, c_szName);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildChangeGradeNamePacket %d, %s\n", byGradeNumber, c_szName);
	return true;
}

bool CPythonNetworkStream::SendGuildChangeGradeAuthorityPacket(BYTE byGradeNumber, BYTE byAuthority)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeChangeGradeAuthority(byGradeNumber, byAuthority);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildChangeGradeAuthorityPacket %d, %d\n", byGradeNumber, byAuthority);
	return true;
}

bool CPythonNetworkStream::SendGuildOfferPacket(DWORD dwExperience)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeOfferExp(dwExperience);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildOfferPacket %d\n", dwExperience);
	return true;
}

bool CPythonNetworkStream::SendGuildPostCommentPacket(const char * c_szMessage)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodePostComment(c_szMessage);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildPostCommentPacket %s\n", c_szMessage);
	return true;
}

bool CPythonNetworkStream::SendGuildDeleteCommentPacket(DWORD dwIndex)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeDeleteComment(dwIndex);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildDeleteCommentPacket %d\n", dwIndex);
	return true;
}

bool CPythonNetworkStream::SendGuildRefreshCommentsPacket(DWORD dwHighestIndex)
{
	static DWORD s_LastTime = timeGetTime() - 1001;

	if (timeGetTime() - s_LastTime < 1000)
		return true;
	s_LastTime = timeGetTime();

	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeRefreshComments();
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildRefreshCommentPacket %d\n", dwHighestIndex);
	return true;
}

bool CPythonNetworkStream::SendGuildChangeMemberGradePacket(DWORD dwPID, BYTE byGrade)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeChangeMemberGrade(dwPID, byGrade);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildChangeMemberGradePacket %d, %d\n", dwPID, byGrade);
	return true;
}

bool CPythonNetworkStream::SendGuildUseSkillPacket(DWORD dwSkillID, DWORD dwTargetVID)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeUseSkill(dwSkillID, dwTargetVID);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildUseSkillPacket %d, %d\n", dwSkillID, dwTargetVID);
	return true;
}

bool CPythonNetworkStream::SendGuildChangeMemberGeneralPacket(DWORD dwPID, BYTE byFlag)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeChangeMemberGeneral(dwPID, byFlag);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildChangeMemberGeneralFlagPacket %d, %d\n", dwPID, byFlag);
	return true;
}

bool CPythonNetworkStream::SendGuildInviteAnswerPacket(DWORD dwGuildID, BYTE byAnswer)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeInviteAnswer(dwGuildID, byAnswer);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildInviteAnswerPacket %d, %d\n", dwGuildID, byAnswer);
	return true;
}

bool CPythonNetworkStream::SendGuildChargeGSPPacket(DWORD dwMoney)
{
	TPacketCGGuild GuildPacket;
	GuildPacket.header = CG::GUILD;
	GuildPacket.length = sizeof(GuildPacket) + sizeof(dwMoney);
	GuildPacket.bySubHeader = GuildSub::CG::CHARGE_GSP;
	if (!Send(sizeof(GuildPacket), &GuildPacket))
		return false;

	if (!Send(sizeof(dwMoney), &dwMoney))
		return false;

	Tracef(" SendGuildChargeGSPPacket %d\n", dwMoney);
	return true;
}

bool CPythonNetworkStream::SendGuildDepositMoneyPacket(DWORD dwMoney)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeDepositMoney(dwMoney);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildDepositMoneyPacket %d\n", dwMoney);
	return true;
}

bool CPythonNetworkStream::SendGuildWithdrawMoneyPacket(DWORD dwMoney)
{
	auto buffer = Client::Network::Handlers::GuildPacketDomainHandler::EncodeWithdrawMoney(dwMoney);
	if (!Send(buffer.size(), buffer.data()))
		return false;

	Tracef(" SendGuildWithdrawMoneyPacket %d\n", dwMoney);
	return true;
}

bool CPythonNetworkStream::RecvGuild()
{
	TPacketGCGuild GuildPacket;
	if (!Recv(sizeof(GuildPacket), &GuildPacket))
		return false;

	static const std::unordered_map<uint8_t, bool (CPythonNetworkStream::*)(const TPacketGCGuild&)> handlers = {
		{ GuildSub::GC::LOGIN,                  &CPythonNetworkStream::RecvGuildSub_Login },
		{ GuildSub::GC::LOGOUT,                 &CPythonNetworkStream::RecvGuildSub_Logout },
		{ GuildSub::GC::REMOVE,                 &CPythonNetworkStream::RecvGuildSub_Remove },
		{ GuildSub::GC::LIST,                   &CPythonNetworkStream::RecvGuildSub_List },
		{ GuildSub::GC::GRADE,                  &CPythonNetworkStream::RecvGuildSub_Grade },
		{ GuildSub::GC::GRADE_NAME,             &CPythonNetworkStream::RecvGuildSub_GradeName },
		{ GuildSub::GC::GRADE_AUTH,             &CPythonNetworkStream::RecvGuildSub_GradeAuth },
		{ GuildSub::GC::INFO,                   &CPythonNetworkStream::RecvGuildSub_Info },
		{ GuildSub::GC::COMMENTS,               &CPythonNetworkStream::RecvGuildSub_Comments },
		{ GuildSub::GC::CHANGE_EXP,             &CPythonNetworkStream::RecvGuildSub_ChangeExp },
		{ GuildSub::GC::CHANGE_MEMBER_GRADE,    &CPythonNetworkStream::RecvGuildSub_ChangeMemberGrade },
		{ GuildSub::GC::SKILL_INFO,             &CPythonNetworkStream::RecvGuildSub_SkillInfo },
		{ GuildSub::GC::CHANGE_MEMBER_GENERAL,  &CPythonNetworkStream::RecvGuildSub_ChangeMemberGeneral },
		{ GuildSub::GC::GUILD_INVITE,           &CPythonNetworkStream::RecvGuildSub_Invite },
		{ GuildSub::GC::WAR,                    &CPythonNetworkStream::RecvGuildSub_War },
		{ GuildSub::GC::GUILD_NAME,             &CPythonNetworkStream::RecvGuildSub_Name },
		{ GuildSub::GC::GUILD_WAR_LIST,         &CPythonNetworkStream::RecvGuildSub_WarList },
		{ GuildSub::GC::GUILD_WAR_END_LIST,     &CPythonNetworkStream::RecvGuildSub_WarEndList },
		{ GuildSub::GC::WAR_POINT,              &CPythonNetworkStream::RecvGuildSub_WarPoint },
		{ GuildSub::GC::MONEY_CHANGE,           &CPythonNetworkStream::RecvGuildSub_MoneyChange },
	};

	auto it = handlers.find(GuildPacket.subheader);
	if (it == handlers.end())
	{
		TraceError("RecvGuild: unknown subheader %d", GuildPacket.subheader);
		return true;
	}
	return (this->*(it->second))(GuildPacket);
}

bool CPythonNetworkStream::RecvGuildSub_Login(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(uint32_t), &dwPID))
		return false;

	// Messenger
	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().LoginGuildMember(pGuildMemberData->strName.c_str());

	//Tracef(" <Login> %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Logout(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(uint32_t), &dwPID))
		return false;

	// Messenger
	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().LogoutGuildMember(pGuildMemberData->strName.c_str());

	//Tracef(" <Logout> %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Remove(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(dwPID), &dwPID))
		return false;

	// Main Player 일 경우 DeleteGuild
	if (CPythonGuild::Instance().IsMainPlayer(dwPID))
	{
		CPythonGuild::Instance().Destroy();
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "DeleteGuild", Py_BuildValue("()"));
		CPythonMessenger::Instance().RemoveAllGuildMember();
		__SetGuildID(0);
		__RefreshMessengerWindow();
		__RefreshTargetBoard();
		__RefreshCharacterWindow();
	}
	else
	{
		// Get Member Name
		std::string strMemberName = "";
		CPythonGuild::TGuildMemberData * pData;
		if (CPythonGuild::Instance().GetMemberDataPtrByPID(dwPID, &pData))
		{
			strMemberName = pData->strName;
			CPythonMessenger::Instance().RemoveGuildMember(pData->strName.c_str());
		}

		CPythonGuild::Instance().RemoveMember(dwPID);

		// Refresh
		__RefreshTargetBoardByName(strMemberName.c_str());
		__RefreshGuildWindowMemberPage();
	}

	Tracef(" <Remove> %d\n", dwPID);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_List(const TPacketGCGuild& pack)
{
	int iPacketSize = int(pack.length) - sizeof(pack);

	for (; iPacketSize > 0;)
	{
		TPacketGCGuildSubMember memberPacket;
		if (!Recv(sizeof(memberPacket), &memberPacket))
			return false;

		char szName[CHARACTER_NAME_MAX_LEN+1] = "";
		if (memberPacket.byNameFlag)
		{
			if (!Recv(sizeof(szName), &szName))
				return false;

			iPacketSize -= CHARACTER_NAME_MAX_LEN+1;
		}
		else
		{
			CPythonGuild::TGuildMemberData * pMemberData;
			if (CPythonGuild::Instance().GetMemberDataPtrByPID(memberPacket.pid, &pMemberData))
			{
				strncpy(szName, pMemberData->strName.c_str(), CHARACTER_NAME_MAX_LEN);
			}
		}

		//Tracef(" <List> %d : %s, %d (%d, %d, %d)\n", memberPacket.pid, szName, memberPacket.byGrade, memberPacket.byJob, memberPacket.byLevel, memberPacket.dwOffer);

		CPythonGuild::SGuildMemberData GuildMemberData;
		GuildMemberData.dwPID = memberPacket.pid;
		GuildMemberData.byGrade = memberPacket.byGrade;
		GuildMemberData.strName = szName;
		GuildMemberData.byJob = memberPacket.byJob;
		GuildMemberData.byLevel = memberPacket.byLevel;
		GuildMemberData.dwOffer = memberPacket.dwOffer;
		GuildMemberData.byGeneralFlag = memberPacket.byIsGeneral;
		CPythonGuild::Instance().RegisterMember(GuildMemberData);

		// Messenger
		if (strcmp(szName, CPythonPlayer::Instance().GetName()))
			CPythonMessenger::Instance().AppendGuildMember(szName);

		__RefreshTargetBoardByName(szName);

		iPacketSize -= sizeof(memberPacket);
	}

	__RefreshGuildWindowInfoPage();
	__RefreshGuildWindowMemberPage();
	__RefreshMessengerWindow();
	__RefreshCharacterWindow();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Grade(const TPacketGCGuild& pack)
{
	BYTE byCount;
	if (!Recv(sizeof(byCount), &byCount))
		return false;

	for (BYTE i = 0; i < byCount; ++ i)
	{
		BYTE byIndex;
		if (!Recv(sizeof(byCount), &byIndex))
			return false;
		TPacketGCGuildSubGrade GradePacket;
		if (!Recv(sizeof(GradePacket), &GradePacket))
			return false;

		auto data = CPythonGuild::SGuildGradeData(GradePacket.auth_flag, GradePacket.grade_name);
		CPythonGuild::Instance().SetGradeData(byIndex, data);
		//Tracef(" <Grade> [%d/%d] : %s, %d\n", byIndex, byCount, GradePacket.grade_name, GradePacket.auth_flag);
	}
	__RefreshGuildWindowGradePage();
	__RefreshGuildWindowMemberPageGradeComboBox();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_GradeName(const TPacketGCGuild& pack)
{
	BYTE byGradeNumber;
	if (!Recv(sizeof(byGradeNumber), &byGradeNumber))
		return false;

	char szGradeName[GUILD_GRADE_NAME_MAX_LEN+1] = "";
	if (!Recv(sizeof(szGradeName), &szGradeName))
		return false;

	CPythonGuild::Instance().SetGradeName(byGradeNumber, szGradeName);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildGrade", Py_BuildValue("()"));

	Tracef(" <Change Grade Name> %d, %s\n", byGradeNumber, szGradeName);
	__RefreshGuildWindowGradePage();
	__RefreshGuildWindowMemberPageGradeComboBox();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_GradeAuth(const TPacketGCGuild& pack)
{
	BYTE byGradeNumber;
	if (!Recv(sizeof(byGradeNumber), &byGradeNumber))
		return false;
	BYTE byAuthorityFlag;
	if (!Recv(sizeof(byAuthorityFlag), &byAuthorityFlag))
		return false;

	CPythonGuild::Instance().SetGradeAuthority(byGradeNumber, byAuthorityFlag);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildGrade", Py_BuildValue("()"));

	Tracef(" <Change Grade Authority> %d, %d\n", byGradeNumber, byAuthorityFlag);
	__RefreshGuildWindowGradePage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Info(const TPacketGCGuild& pack)
{
	TPacketGCGuildInfo GuildInfo;
	if (!Recv(sizeof(GuildInfo), &GuildInfo))
		return false;

	if (!PhaseGameGuildBridge::HandleGuildSub_Info(GuildInfo))
		return false;

	__RefreshGuildWindowInfoPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Comments(const TPacketGCGuild& pack)
{
	BYTE byCount;
	if (!Recv(sizeof(byCount), &byCount))
		return false;

	CPythonGuild::Instance().ClearComment();
	//Tracef(" >>> Comments Count : %d\n", byCount);

	for (BYTE i = 0; i < byCount; ++i)
	{
		DWORD dwCommentID;
		if (!Recv(sizeof(dwCommentID), &dwCommentID))
			return false;

		char szName[CHARACTER_NAME_MAX_LEN+1] = "";
		if (!Recv(sizeof(szName), &szName))
			return false;

		char szComment[GULID_COMMENT_MAX_LEN+1] = "";
		if (!Recv(sizeof(szComment), &szComment))
			return false;

		//Tracef(" [Comment-%d] : %s, %s\n", dwCommentID, szName, szComment);
		CPythonGuild::Instance().RegisterComment(dwCommentID, szName, szComment);
	}

	__RefreshGuildWindowBoardPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_ChangeExp(const TPacketGCGuild& pack)
{
	BYTE byLevel;
	if (!Recv(sizeof(byLevel), &byLevel))
		return false;
	uint32_t dwEXP;
	if (!Recv(sizeof(dwEXP), &dwEXP))
		return false;
	CPythonGuild::Instance().SetGuildEXP(byLevel, dwEXP);
	Tracef(" <ChangeEXP> %d, %d\n", byLevel, dwEXP);
	__RefreshGuildWindowInfoPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_ChangeMemberGrade(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(dwPID), &dwPID))
		return false;
	BYTE byGrade;
	if (!Recv(sizeof(byGrade), &byGrade))
		return false;
	CPythonGuild::Instance().ChangeGuildMemberGrade(dwPID, byGrade);
	Tracef(" <ChangeMemberGrade> %d, %d\n", dwPID, byGrade);
	__RefreshGuildWindowMemberPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_SkillInfo(const TPacketGCGuild& pack)
{
	CPythonGuild::TGuildSkillData & rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();
	if (!Recv(sizeof(rSkillData.bySkillPoint), &rSkillData.bySkillPoint))
		return false;
	if (!Recv(sizeof(rSkillData.bySkillLevel), rSkillData.bySkillLevel))
		return false;
	if (!Recv(sizeof(rSkillData.wGuildPoint), &rSkillData.wGuildPoint))
		return false;
	if (!Recv(sizeof(rSkillData.wMaxGuildPoint), &rSkillData.wMaxGuildPoint))
		return false;

	Tracef(" <SkillInfo> %d / %d, %d\n", rSkillData.bySkillPoint, rSkillData.wGuildPoint, rSkillData.wMaxGuildPoint);
	__RefreshGuildWindowSkillPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_ChangeMemberGeneral(const TPacketGCGuild& pack)
{
	uint32_t dwPID;
	if (!Recv(sizeof(dwPID), &dwPID))
		return false;
	BYTE byFlag;
	if (!Recv(sizeof(byFlag), &byFlag))
		return false;

	CPythonGuild::Instance().ChangeGuildMemberGeneralFlag(dwPID, byFlag);
	Tracef(" <ChangeMemberGeneralFlag> %d, %d\n", dwPID, byFlag);
	__RefreshGuildWindowMemberPage();
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Invite(const TPacketGCGuild& pack)
{
	uint32_t dwGuildID;
	if (!Recv(sizeof(dwGuildID), &dwGuildID))
		return false;
	char szGuildName[GUILD_NAME_MAX_LEN+1];
	if (!Recv(GUILD_NAME_MAX_LEN, &szGuildName))
		return false;

	szGuildName[GUILD_NAME_MAX_LEN] = 0;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RecvGuildInviteQuestion", Py_BuildValue("(is)", dwGuildID, szGuildName));
	Tracef(" <Guild Invite> %d, %s\n", dwGuildID, szGuildName);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_War(const TPacketGCGuild& pack)
{
	TPacketGCGuildWar kGuildWar;
	if (!Recv(sizeof(kGuildWar), &kGuildWar))
		return false;

	switch (kGuildWar.bWarState)
	{
		case GUILD_WAR_SEND_DECLARE:
			Tracef(" >> GuildSub::GC::WAR : GUILD_WAR_SEND_DECLARE\n");
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnSendDeclare",
				Py_BuildValue("(i)", kGuildWar.dwGuildOpp)
			);
			break;
		case GUILD_WAR_RECV_DECLARE:
			Tracef(" >> GuildSub::GC::WAR : GUILD_WAR_RECV_DECLARE\n");
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnRecvDeclare",
				Py_BuildValue("(ii)", kGuildWar.dwGuildOpp, kGuildWar.bType)
			);
			break;
		case GUILD_WAR_ON_WAR:
			Tracef(" >> GuildSub::GC::WAR : GUILD_WAR_ON_WAR : %d, %d\n", kGuildWar.dwGuildSelf, kGuildWar.dwGuildOpp);
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnStart",
				Py_BuildValue("(ii)", kGuildWar.dwGuildSelf, kGuildWar.dwGuildOpp)
			);
			CPythonGuild::Instance().StartGuildWar(kGuildWar.dwGuildOpp);
			break;
		case GUILD_WAR_END:
			Tracef(" >> GuildSub::GC::WAR : GUILD_WAR_END\n");
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnEnd",
				Py_BuildValue("(ii)", kGuildWar.dwGuildSelf, kGuildWar.dwGuildOpp)
			);
			CPythonGuild::Instance().EndGuildWar(kGuildWar.dwGuildOpp);
			break;
	}
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_Name(const TPacketGCGuild& pack)
{
	uint32_t dwID;
	char szGuildName[GUILD_NAME_MAX_LEN+1];

	int iPacketSize = int(pack.length) - sizeof(pack);

	int nItemSize = sizeof(dwID) + GUILD_NAME_MAX_LEN;

	assert(iPacketSize%nItemSize==0 && "GuildSub::GC::GUILD_NAME");

	for (; iPacketSize > 0;)
	{
		if (!Recv(sizeof(dwID), &dwID))
			return false;

		if (!Recv(GUILD_NAME_MAX_LEN, &szGuildName))
			return false;

		szGuildName[GUILD_NAME_MAX_LEN] = 0;

		//Tracef(" >> GulidName [%d : %s]\n", dwID, szGuildName);
		CPythonGuild::Instance().RegisterGuildName(dwID, szGuildName);
		iPacketSize -= nItemSize;
	}
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_WarList(const TPacketGCGuild& pack)
{
	uint32_t dwSrcGuildID;
	uint32_t dwDstGuildID;

	int iPacketSize = int(pack.length) - sizeof(pack);
	int nItemSize = sizeof(dwSrcGuildID) + sizeof(dwDstGuildID);

	assert(iPacketSize%nItemSize==0 && "GuildSub::GC::GUILD_WAR_LIST");

	for (; iPacketSize > 0;)
	{
		if (!Recv(sizeof(dwSrcGuildID), &dwSrcGuildID))
			return false;

		if (!Recv(sizeof(dwDstGuildID), &dwDstGuildID))
			return false;

		Tracef(" >> GulidWarList [%d vs %d]\n", dwSrcGuildID, dwDstGuildID);
		CInstanceBase::InsertGVGKey(dwSrcGuildID, dwDstGuildID);
		CPythonCharacterManager::Instance().ChangeGVG(dwSrcGuildID, dwDstGuildID);
		iPacketSize -= nItemSize;
	}
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_WarEndList(const TPacketGCGuild& pack)
{
	uint32_t dwSrcGuildID;
	uint32_t dwDstGuildID;

	int iPacketSize = int(pack.length) - sizeof(pack);
	int nItemSize = sizeof(dwSrcGuildID) + sizeof(dwDstGuildID);

	assert(iPacketSize%nItemSize==0 && "GuildSub::GC::GUILD_WAR_END_LIST");

	for (; iPacketSize > 0;)
	{

		if (!Recv(sizeof(dwSrcGuildID), &dwSrcGuildID))
			return false;

		if (!Recv(sizeof(dwDstGuildID), &dwDstGuildID))
			return false;

		Tracef(" >> GulidWarEndList [%d vs %d]\n", dwSrcGuildID, dwDstGuildID);
		CInstanceBase::RemoveGVGKey(dwSrcGuildID, dwDstGuildID);
		CPythonCharacterManager::Instance().ChangeGVG(dwSrcGuildID, dwDstGuildID);
		iPacketSize -= nItemSize;
	}
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_WarPoint(const TPacketGCGuild& pack)
{
	TPacketGuildWarPoint GuildWarPoint;
	if (!Recv(sizeof(GuildWarPoint), &GuildWarPoint))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
		"BINARY_GuildWar_OnRecvPoint",
		Py_BuildValue("(iii)", GuildWarPoint.dwGainGuildID, GuildWarPoint.dwOpponentGuildID, GuildWarPoint.lPoint)
	);
	return true;
}

bool CPythonNetworkStream::RecvGuildSub_MoneyChange(const TPacketGCGuild& pack)
{
	uint32_t dwMoney;
	if (!Recv(sizeof(dwMoney), &dwMoney))
		return false;

	CPythonGuild::Instance().SetGuildMoney(dwMoney);

	__RefreshGuildWindowInfoPage();
	Tracef(" >> Guild Money Change : %d\n", dwMoney);
	return true;
}

static DWORD gs_dwMarkUpdateRequestTime = 0;

bool CPythonNetworkStream::RecvMarkUpdate()
{
	TPacketGCMarkUpdate packet;
	if (!Recv(sizeof(packet), &packet))
		return false;

	Tracef(" >> RecvMarkUpdate: guildID=%u, imgIdx=%u\n", packet.guildID, packet.imgIdx);

	CGuildMarkManager::Instance().ReloadMarkImage(packet.imgIdx);

	// Rate limit mark downloads to prevent connection spam from multiple simultaneous guild uploads
	// Allow at most one download request per 1 second from server-pushed updates
	DWORD dwCurrentTime = ELTimer_GetMSec();
	if (dwCurrentTime < gs_dwMarkUpdateRequestTime)
		return true;

	gs_dwMarkUpdateRequestTime = dwCurrentTime + 1000; // 1 second cooldown

	CGuildMarkDownloader& rkGuildMarkDownloader = CGuildMarkDownloader::Instance();
	rkGuildMarkDownloader.Connect(m_kMarkAuth.m_kNetAddr, m_kMarkAuth.m_dwHandle, m_kMarkAuth.m_dwRandomKey);

	return true;
}

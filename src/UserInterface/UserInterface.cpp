#include "StdAfx.h"
#include "PythonApplication.h"
#include "ProcessScanner.h"
#include "PythonExceptionSender.h"
#include "resource.h"
#include "Version.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include "eterLib/Util.h"
#include "EterLib/GameThreadPool.h"
#include "EterBase/lzo.h"

#include "PackLib/PackManager.h"

#include <filesystem>
#include <format>
#include <thread>
#include <atomic>
#include <algorithm>
#include <unordered_set>

static bool g_bEagerPackLoading = false;
static bool g_bNoPythonMode = false;
static std::thread s_worldPackThread;
static std::atomic<bool> s_bWorldPacksReady{false};

#include <stdlib.h>
#include <utf8.h>
#include <sodium.h>
#include "EterLib/FontManager.h"
#include "TestHarness/CrashSentinel.h"
#include "TestHarness/DeterministicTickController.h"
#include "TestHarness/MockWorldDriver.h"
#include "TestHarness/TestHarnessServer.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/ServerProfileManager.h"
#include "FastBootOverlay.h"
#include <fstream>
#include <sstream>
#include "rapidjson/document.h"

extern "C" {
	extern int _fltused;
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
	volatile int _AVOID_FLOATING_POINT_LIBRARY_BUG = _fltused;
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
};

#pragma comment(linker, "/NODEFAULTLIB:libci.lib")

bool g_bFastBootAutoConnect = false;
bool g_bAutoReconnect = false;
DWORD g_dwReconnectDelayMs = 3000;
int g_iDirectEnterSlot = -1;

int Setup(LPSTR lpCmdLine);
static const char * sc_apszPythonLibraryFilenames[] =
{
	"UserDict.pyc",
	"__future__.pyc",
	"copy_reg.pyc",
	"linecache.pyc",
	"ntpath.pyc",
	"os.pyc",
	"site.pyc",
	"stat.pyc",
	"string.pyc",
	"traceback.pyc",
	"types.pyc",
	"\n",
};

bool PackInitialize(const char * c_pszFolder)
{
	std::string folder = (c_pszFolder && *c_pszFolder) ? c_pszFolder : "pack";

	if (_access(folder.c_str(), 0) != 0)
	{
		if (_access("../../m2dev-client-main/pack", 0) == 0)
		{
			folder = "../../m2dev-client-main/pack";
		}
		else if (_access("m2dev-client-main/pack", 0) == 0)
		{
			folder = "m2dev-client-main/pack";
		}
		else
		{
			if (g_bHeadlessMode)
			{
				TraceError("[HEADLESS] Ostrzezenie: Folder zasobow pack nie istnieje ('%s'). Kontynuacja w trybie mock/headless.", folder.c_str());
				return true;
			}
			return false;
		}
	}

	Tracef("PackInitialize: Loading root.pck\n");
	DWORD dwStartTime = GetTickCount();
	std::string rootPath = (std::filesystem::path(folder) / "root.pck").generic_string();
	if (!CPackManager::instance().AddPack(rootPath))
	{
		if (g_bHeadlessMode)
		{
			TraceError("[HEADLESS] Ostrzezenie: Nie udalo sie zaladowac root.pck ze sciezki '%s'. Kontynuacja w trybie mock/headless.", folder.c_str());
			return true;
		}
		TraceError("Failed to load root.pck");
		return false;
	}

	std::vector<std::string> packFiles;
	std::error_code ec;

	try
	{
		if (std::filesystem::exists(folder, ec) && std::filesystem::is_directory(folder, ec))
		{
			for (const auto& entry : std::filesystem::directory_iterator(folder, ec))
			{
				if (ec)
				{
					TraceError("PackInitialize: Blad podczas iteracji po folderze pack: %s\n", ec.message().c_str());
					break;
				}

				if (entry.is_regular_file(ec))
				{
					std::string ext = entry.path().extension().string();
					if (_stricmp(ext.c_str(), ".pck") == 0)
					{
						std::string filename = entry.path().filename().string();
						if (_stricmp(filename.c_str(), "root.pck") == 0)
						{
							continue; // root.pck zostal zaladowany jako pierwszy
						}
						packFiles.push_back(entry.path().generic_string());
					}
				}
			}
		}
		else
		{
			TraceError("PackInitialize: Folder '%s' nie istnieje lub nie jest katalogiem\n", folder.c_str());
		}
	}
	catch (const std::exception& ex)
	{
		TraceError("PackInitialize: Wyjatek filesystem podczas skanowania katalogu pack: %s\n", ex.what());
		packFiles.clear();
	}

	// W przypadku bledu systemu plikow lub braku plikow w folderze, zachowaj bezpieczna minimalna liste rezerwowa (root, locale, ETC, uiscript)
	if (packFiles.empty())
	{
		Tracef("PackInitialize: Brak dodatkowych plikow .pck w folderze '%s' lub blad skanowania. Uzycie listy rezerwowej.\n", folder.c_str());
		const std::vector<std::string> fallbackPacks = {
			"root",
			"locale",
			"ETC",
			"uiscript"
		};

		for (const auto& fallbackName : fallbackPacks)
		{
			if (_stricmp(fallbackName.c_str(), "root") == 0)
			{
				continue; // root.pck zostal juz zaladowany
			}
			packFiles.push_back((std::filesystem::path(folder) / (fallbackName + ".pck")).generic_string());
		}
	}
	else
	{
		std::sort(packFiles.begin(), packFiles.end());
	}

	// Deterministyczne synchroniczne zaladowanie wszystkich paczek (Zero Race Condition)
	for (const auto& packPath : packFiles)
	{
		CPackManager::instance().AddPack(packPath);
	}
	s_bWorldPacksReady.store(true, std::memory_order_release);

	Tracef("PackInitialize: Zaladowano %zu paczek w %d ms (Data-Driven VFS)\n", packFiles.size(), GetTickCount() - dwStartTime);
	return true;
}

bool RunMainScript(CPythonLauncher& pyLauncher, const char* lpCmdLine)
{
	initpack();
	initdbg();
	initime();
	initgrp();
	initgrpImage();
	initgrpText();
	initwndMgr();

	initapp();
	initsystem();
	initchr();
	initchrmgr();
	initPlayer();
	initItem();
	initNonPlayer();
	initTrade();
	initChat();
	initTextTail();
	initnet();
	initMiniMap();
	initProfiler();
	initEvent();
	initeffect();
	initfly();
	initsnd();
	initeventmgr();
	initshop();
	initskill();
	initquest();
	initBackground();
	initMessenger();
	initsafebox();
	initguild();
	initServerStateChecker();
	initdebugRender();
	std::string stRegisterDebugFlag;

	#ifdef _DISTRIBUTE
	stRegisterDebugFlag = "__DEBUG__ = 0";
	#else
	stRegisterDebugFlag = "__DEBUG__ = 1";
	#endif

	if (!pyLauncher.RunLine(stRegisterDebugFlag.c_str()))
	{
		TraceError("RegisterDebugFlag Error");
		return false;
	}

	if (!pyLauncher.RunFile("system.py"))
	{
		TraceError("RunMain Error");
		return false;
	}

	return true;
}

static bool Main(HINSTANCE hInstance, LPSTR lpCmdLine)
{
	DWORD dwRandSeed = (DWORD)time(NULL) ^ GetCurrentProcessId() ^ GetTickCount();
	srandom(dwRandSeed);
	srand(random());
	SetLogLevel(1);

	if (!Setup(lpCmdLine))
		return false;

#ifdef _DEBUG
	OpenConsoleWindow();
#endif
	OpenLogFile(true); // Always generate full log.txt and syserr.txt


	if (sodium_init() < 0)
	{
		LogBox("sodium_init() failed");
		return false;
	}

	if (!g_bHeadlessMode)
	{
		if (!CFontManager::Instance().Initialize())
		{
			LogBox("FreeType initialization failed");
			return false;
		}
	}

	static CLZO lzo;
	CPackManager packMgr;

	if (!PackInitialize("pack"))
	{
		LogBox("Pack Initialization failed. Check log.txt file..");
		CFontManager::Instance().Destroy();
		timeEndPeriod(1);
		return false;
	}

	// Create game thread pool singleton before CPythonApplication
	static CGameThreadPool gameThreadPool;

	if (g_bNoPythonMode)
	{
		TraceError("[DOMAIN_MODE] Uruchamianie w trybie Pure C++ Domain Mode (--no-python Zero-Python Headless)");
		auto app = new CPythonApplication;
		if (g_iDirectEnterSlot >= 0)
		{
			CPythonNetworkStream::Instance().SetDirectEnterMode(g_iDirectEnterSlot);
		}
		app->Initialize(hInstance);

		if (CPythonApplication::GetTurboTicks() > 0)
		{
			app->SetTurboTicks(CPythonApplication::GetTurboTicks());
		}

		app->Loop();

		app->Clear();
		app->Destroy();
		delete app;
		app = nullptr;
	}
	else
	{
		CPythonLauncher pyLauncher;

		if (pyLauncher.Create())
		{
			auto app = new CPythonApplication;
			if (g_iDirectEnterSlot >= 0)
			{
				CPythonNetworkStream::Instance().SetDirectEnterMode(g_iDirectEnterSlot);
			}
			app->Initialize(hInstance);

			if (CPythonApplication::GetTurboTicks() > 0)
			{
				app->SetTurboTicks(CPythonApplication::GetTurboTicks());
			}

			if (!RunMainScript(pyLauncher, lpCmdLine))
			{
				TraceError("Main: RunMainScript failed!");
				if (app && (app->GetTurboTicks() > 0 || g_bHeadlessMode || (lpCmdLine && (strstr(lpCmdLine, "--test-mode") || strstr(lpCmdLine, "--mock-world") || strstr(lpCmdLine, "--freeze-on-start")))))
				{
					TraceError("Main: Headless / Test-mode active, running loop directly for TestHarnessServer IPC...");
					app->Loop();
				}
			}
			else
			{
				TraceError("Main: RunMainScript finished!");
			}

			// Filar A: Najpierw zwalniamy srodowisko Pythona, aby obiekty i destruktory ui zostaly wyczyszczone przed zniszczeniem instancji aplikacji
			pyLauncher.Clear();

			app->Clear();
			app->Destroy();
			delete app;
			app = nullptr;
		}
	}

	if (s_worldPackThread.joinable())
	{
		s_worldPackThread.join();
	}

	timeEndPeriod(1);

	if (!g_bHeadlessMode)
	{
		CFontManager::Instance().Destroy();
	}
	return 0;
}

void __ErrorPythonLibraryIsNotExist()
{
	LogBoxf("FATAL ERROR!! Python Library file not exist!");
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UserInterface::TestHarness::CrashSentinel::Instance().Initialize();

	if (lpCmdLine && (strstr(lpCmdLine, "--headless") || strstr(lpCmdLine, "--turbo-ticks") || strstr(lpCmdLine, "__COMMAND_TURBO_TICKS__")))
	{
		SetHeadlessMode(true);
	}

	if (lpCmdLine && strstr(lpCmdLine, "--no-python"))
	{
		g_bNoPythonMode = true;
	}

	if (lpCmdLine && (strstr(lpCmdLine, "--eager-packs") || strstr(lpCmdLine, "--eager")))
	{
		g_bEagerPackLoading = true;
	}

	if (lpCmdLine && strstr(lpCmdLine, "--stealth"))
	{
		SetWindowVisible(false);
	}

	if (lpCmdLine)
	{
		auto extractArgVal = [](const char* cmdLine, const char* flag) -> std::string {
			if (!cmdLine || !flag) return "";
			const char* p = cmdLine;
			size_t flagLen = strlen(flag);
			while ((p = strstr(p, flag)) != nullptr)
			{
				// Upewniamy sie ze p jest poczatkiem argumentu
				if (p != cmdLine && *(p - 1) != ' ' && *(p - 1) != '\t')
				{
					p += flagLen;
					continue;
				}
				// Sprawdzamy co nastepuje po fladze: musi to byc '=' lub spacja lub koniec
				const char* next = p + flagLen;
				if (*next != '=' && *next != ' ' && *next != '\t' && *next != '\0')
				{
					p += flagLen;
					continue;
				}

				const char* start = next;
				while (*start == ' ' || *start == '\t' || *start == '=') ++start;
				// Jesli kolejna wartosc to poczatek innej flagi, biezaca flaga byla bezargumentowa
				if (*start == '-' && *(start + 1) == '-')
				{
					return "";
				}
				if (*start == '"') {
					++start;
					const char* end = strchr(start, '"');
					return end ? std::string(start, end) : std::string(start);
				}
				const char* end = start;
				while (*end && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n')
					++end;
				return std::string(start, end);
			}
			return "";
		};

		auto activateDriverWithFallback = [](std::string driverName) -> bool {
			if (driverName.empty())
				return false;
			auto& reg = ::Network::Protocol::ProtocolDriverRegistry::Instance();
			if (!reg.GetActiveDriver())
			{
				reg.InitializeDefaults();
			}
			if (reg.SetActiveDriver(driverName))
				return true;

			std::string lower = driverName;
			for (char& c : lower) c = (char)tolower(c);
			if (reg.SetActiveDriver(lower))
				return true;

			if (lower.find("beavium") != std::string::npos)
				return reg.SetActiveDriver("beavium");
			if (lower.find("marty") != std::string::npos || lower.find("pandora") != std::string::npos)
				return reg.SetActiveDriver("martysama");
			if (lower.find("luria") != std::string::npos)
				return reg.SetActiveDriver("luria");
			return reg.SetActiveDriver("standard_x64");
		};

		// --turbo-ticks=<N> lub __COMMAND_TURBO_TICKS__=<N>
		std::string turboTicksStr = extractArgVal(lpCmdLine, "--turbo-ticks");
		if (turboTicksStr.empty())
		{
			turboTicksStr = extractArgVal(lpCmdLine, "__COMMAND_TURBO_TICKS__");
		}
		if (turboTicksStr.empty())
		{
			const char* customCmd = strstr(lpCmdLine, "__COMMAND_TURBO_TICKS__");
			if (customCmd)
			{
				customCmd += strlen("__COMMAND_TURBO_TICKS__");
				while (*customCmd == '=' || *customCmd == ' ' || *customCmd == '\t')
					++customCmd;
				if (isdigit(static_cast<unsigned char>(*customCmd)))
				{
					turboTicksStr = customCmd;
				}
			}
		}

		if (!turboTicksStr.empty())
		{
			char* endPtr = nullptr;
			unsigned long parsedTicks = std::strtoul(turboTicksStr.c_str(), &endPtr, 10);
			if (parsedTicks > 0)
			{
				CPythonApplication::SetTurboTicks(static_cast<uint32_t>(parsedTicks));
			}
		}

		// --server=<name>
		std::string serverName = extractArgVal(lpCmdLine, "--server");
		if (!serverName.empty())
		{
			activateDriverWithFallback(serverName);
		}

		// --profile=<path>
		std::string profilePath = extractArgVal(lpCmdLine, "--profile");
		if (!profilePath.empty())
		{
			Client::Network::ServerProfileManager profileMgr;
			auto loadRes = profileMgr.LoadProfile(profilePath);
			if (loadRes.has_value())
			{
				std::string profName;
				auto activeProfRes = profileMgr.GetActiveProfile();
				if (activeProfRes.has_value())
				{
					profName = activeProfRes.value().name;
				}

				std::string driverName = profName;
				std::ifstream f(profilePath);
				if (f.is_open())
				{
					std::stringstream ss;
					ss << f.rdbuf();
					rapidjson::Document doc;
					if (!doc.Parse(ss.str().c_str()).HasParseError() && doc.IsObject())
					{
						if (doc.HasMember("name") && doc["name"].IsString() && profName.empty())
						{
							profName = doc["name"].GetString();
							driverName = profName;
							(void)profileMgr.SetActiveProfile(profName);
						}
						if (doc.HasMember("driver") && doc["driver"].IsString())
						{
							driverName = doc["driver"].GetString();
						}
					}
				}

				if (!driverName.empty())
				{
					activateDriverWithFallback(driverName);
				}
			}
		}

		if (strstr(lpCmdLine, "--fastboot") != nullptr || strstr(lpCmdLine, "--auto-connect") != nullptr || g_bHeadlessMode)
		{
			g_bFastBootAutoConnect = true;
		}

		if (strstr(lpCmdLine, "--auto-reconnect") != nullptr)
		{
			g_bAutoReconnect = true;
		}

		std::string reconDelayStr = extractArgVal(lpCmdLine, "--reconnect-delay");
		if (!reconDelayStr.empty())
		{
			try { g_dwReconnectDelayMs = static_cast<DWORD>(std::stoul(reconDelayStr) * 1000); } catch (...) {}
		}

		if (g_bAutoReconnect)
		{
			FastBootOverlay::Instance().SetAutoReconnect(true, g_dwReconnectDelayMs);
		}

		std::string directSlotStr = extractArgVal(lpCmdLine, "--direct");
		if (!directSlotStr.empty())
		{
			int slot = 0;
			try { slot = std::stoi(directSlotStr); } catch (...) { slot = 0; }
			g_iDirectEnterSlot = slot;
		}
	}

	// Filar 4: TestHarnessEngine - Inicjalizacja CrashSentinel na wypadek access violation
	UserInterface::TestHarness::DeterministicTickController::Instance().InitFromCommandLine(lpCmdLine);
	UserInterface::TestHarness::MockWorldDriver::Instance().InitFromCommandLine(lpCmdLine);
	UserInterface::TestHarness::TestHarnessServer::Instance().Start();

	LoadConfig("config/locale.cfg");

	int nArgc = 0;
	auto szArgv = CommandLineToArgv (lpCmdLine, &nArgc);

	Main (hInstance, lpCmdLine);
	::CoUninitialize();

Clean:
	UserInterface::TestHarness::TestHarnessServer::Instance().Stop();
	UserInterface::TestHarness::CrashSentinel::Instance().Shutdown();
	SAFE_FREE_GLOBAL (szArgv);
	return 0;
}

int Setup(LPSTR lpCmdLine)
{
	TIMECAPS tc;
	UINT wTimerRes;

	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) 
		return 0;

	wTimerRes = MINMAX(tc.wPeriodMin, 1, tc.wPeriodMax); 
	timeBeginPeriod(wTimerRes); 

	return 1;
}

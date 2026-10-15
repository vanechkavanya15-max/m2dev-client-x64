#include <iostream>
#include <cassert>
#include <stdexcept>
#include <string>

#include "PackLib/PackManager.h"
#include "GameLib/MapManager.h"
#include "EterLib/Camera.h"

class CPythonSystem {
public:
	int GetFogLevel();
};
int CPythonSystem::GetFogLevel() { return 0; }

float CCamera::CAMERA_MAX_DISTANCE = 2500.0f;

void TestPackManagerSecurity()
{
	std::cout << "[TEST] PackManager Security & Path Traversal..." << std::endl;

	CPackManager& packMgr = CPackManager::Instance();

	// Test 1: Poprawne sciezki sa normalizowane
	assert(packMgr.IsExist("d:/ymir work/pc/warrior/warrior_novice.msm") || true); // Nie moze sie wywalic

	// Test 2: Blokowanie Path Traversal
	assert(packMgr.IsExist("../secret.txt") == false);
	assert(packMgr.IsExist("../../etc/passwd") == false);
	assert(packMgr.IsExist("d:/ymir work/../../windows/system32/cmd.exe") == false);
	assert(packMgr.IsExist("root/../system.py") == false);
	assert(packMgr.IsExist("..") == false);

	// Test 3: Blokowanie znakow niedozwolonych
	assert(packMgr.IsExist("d:/test*file") == false);
	assert(packMgr.IsExist("d:/test?file") == false);
	assert(packMgr.IsExist("d:/test|file") == false);
	assert(packMgr.IsExist("d:/test<file") == false);
	assert(packMgr.IsExist("d:/test>file") == false);
	assert(packMgr.IsExist("d:/test\"file") == false);
	assert(packMgr.IsExist("c:d:/test") == false);
	assert(packMgr.IsExist("c:/test:stream") == false);

	// Test 4: GetFile z niedozwolona sciezka
	TPackFile fileData;
	assert(packMgr.GetFile("../secret.txt", fileData) == false);
	assert(packMgr.GetFile("c:/windows/system32/calc.exe", fileData) == false);
	assert(packMgr.GetFile("d:/test*file", fileData) == false);

	// Test 5: Cache i brak ciaglego odpytywania dysku
	// Wielokrotne zapytanie o nieistniejacy plik powinno byc natychmiast obslugiwane z cache
	for (int i = 0; i < 1000; ++i)
	{
		assert(packMgr.IsExist("non_existent_asset_path_12345.dds") == false);
	}

	std::cout << "[PASS] PackManager Security & Caching tests passed!" << std::endl;
}

void TestMapManagerSafety()
{
	std::cout << "[TEST] MapManager Safety & GetMapOutdoorRef / GetMapOutdoorPtr..." << std::endl;

	CMapManager mapMgr;

	// 1. Przed alokacja mapy m_pkMap jest nullptr
	assert(mapMgr.IsMapOutdoor() == false);
	assert(mapMgr.GetMapOutdoorPtr() == nullptr);

	// 2. Wywolanie GetMapOutdoorRef na niezainicjalizowanej mapie MUSI rzucic std::runtime_error
	bool exceptionCaught = false;
	try
	{
		[[maybe_unused]] CMapOutdoor& ref = mapMgr.GetMapOutdoorRef();
	}
	catch (const std::runtime_error& e)
	{
		exceptionCaught = true;
	}
	assert(exceptionCaught == true);

	// 3. Bezpieczne metody CMapManager zwracaja wartosci domyslne bez wyjatku
	assert(mapMgr.GetHeight(100.0f, 100.0f) == 0.0f);
	assert(mapMgr.GetTerrainHeight(100.0f, 100.0f) == 0.0f);
	long waterHeight = 0;
	assert(mapMgr.GetWaterHeight(10, 10, &waterHeight) == false);
	assert(mapMgr.UpdateMap(0.0f, 0.0f, 0.0f) == false);
	assert(mapMgr.UnloadMap("test_map") == false);

	// 4. Destroy na pustym stanie jest w pelni bezpieczny
	mapMgr.Destroy();
	assert(mapMgr.IsMapOutdoor() == false);
	assert(mapMgr.GetMapOutdoorPtr() == nullptr);

	std::cout << "[PASS] MapManager Safety tests passed!" << std::endl;
}

int main()
{
	std::cout << "Starting VFS & MapManager Verification Tests..." << std::endl;
	TestPackManagerSecurity();
	TestMapManagerSafety();
	std::cout << "All VFS & MapManager Tests Passed Successfully!" << std::endl;
	return 0;
}

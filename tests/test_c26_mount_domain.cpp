#include "../src/Client/Gameplay/MountDomain.cpp"
#include <iostream>
#include <cassert>

using namespace Client::Gameplay;

void TestMountSuccess()
{
    auto domain = CreateMountDomain();
    assert(!domain->IsMounting());
    assert(domain->GetMountLevel() == 0);
    assert(domain->GetSpeedBonus() == 0);

    auto result = domain->Mount(20101);
    assert(result.has_value());
    assert(domain->IsMounting());
    
    assert(domain->GetMountLevel() == 1);
    assert(domain->GetSpeedBonus() == 30); 
    
    std::cout << "TestMountSuccess passed.\n";
}

void TestDismount()
{
    auto domain = CreateMountDomain();
    domain->Mount(20104);
    
    assert(domain->GetMountLevel() == 2);
    assert(domain->GetSpeedBonus() == 60);

    auto result = domain->Dismount();
    assert(result.has_value());
    assert(!domain->IsMounting());
    assert(domain->GetMountLevel() == 0);
    assert(domain->GetSpeedBonus() == 0);

    std::cout << "TestDismount passed.\n";
}

void TestMountLevelAndSpeed()
{
    auto domain = CreateMountDomain();
    
    domain->Mount(20107);
    assert(domain->GetMountLevel() == 3);
    assert(domain->GetSpeedBonus() == 90);
    
    domain->Dismount();
    
    domain->Mount(20119);
    assert(domain->GetMountLevel() == 2);
    assert(domain->GetSpeedBonus() == 60);

    std::cout << "TestMountLevelAndSpeed passed.\n";
}

void TestInvalidStateTransition()
{
    auto domain = CreateMountDomain();
    
    auto res1 = domain->CanChangeMotionMode(CRaceMotionData::MODE_HORSE);
    assert(!res1.has_value());
    assert(res1.error() == "Cannot change to horse mode without mounting");

    domain->Mount(20101);

    auto res2 = domain->CanChangeMotionMode(CRaceMotionData::MODE_GENERAL);
    assert(!res2.has_value());
    assert(res2.error() == "Cannot change to non-horse mode while mounting");

    auto res3 = domain->CanChangeMotionMode(CRaceMotionData::MODE_HORSE_BOW);
    assert(res3.has_value());

    std::cout << "TestInvalidStateTransition passed.\n";
}

int main()
{
    TestMountSuccess();
    TestDismount();
    TestMountLevelAndSpeed();
    TestInvalidStateTransition();
    
    std::cout << "All MountDomain tests passed!\n";
    return 0;
}

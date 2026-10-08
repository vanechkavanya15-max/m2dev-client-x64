#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include "../src/Client/World/SpatialHashGrid.h"
#include <algorithm>

using namespace Client::World;

TEST_CASE("SpatialHashGrid Basic Operations") {
    SpatialHashGrid grid(1024.0f);
    
    SUBCASE("Insert and Query") {
        EterBase::EntityId id1{1};
        grid.Insert(id1, 500.0f, 500.0f);
        
        auto result = grid.QueryRadius(500.0f, 500.0f, 100.0f);
        CHECK(result.size() == 1);
        CHECK(result[0] == id1);
        
        result = grid.QueryRadius(1500.0f, 1500.0f, 100.0f);
        CHECK(result.size() == 0);
    }
    
    SUBCASE("Update Position") {
        EterBase::EntityId id2{2};
        grid.Insert(id2, 100.0f, 100.0f);
        
        auto result = grid.QueryRadius(100.0f, 100.0f, 50.0f);
        CHECK(result.size() == 1);
        
        grid.Update(id2, 2000.0f, 2000.0f);
        
        result = grid.QueryRadius(100.0f, 100.0f, 50.0f);
        CHECK(result.size() == 0);
        
        result = grid.QueryRadius(2000.0f, 2000.0f, 50.0f);
        CHECK(result.size() == 1);
        CHECK(result[0] == id2);
    }
    
    SUBCASE("Remove Entity") {
        EterBase::EntityId id3{3};
        grid.Insert(id3, 300.0f, 300.0f);
        
        auto result = grid.QueryRadius(300.0f, 300.0f, 50.0f);
        CHECK(result.size() == 1);
        
        grid.Remove(id3);
        
        result = grid.QueryRadius(300.0f, 300.0f, 50.0f);
        CHECK(result.size() == 0);
    }
    
    SUBCASE("Radius Query across Cells") {
        EterBase::EntityId id4{4};
        EterBase::EntityId id5{5};
        
        grid.Insert(id4, 1000.0f, 1000.0f); // cell (0,0)
        grid.Insert(id5, 1100.0f, 1000.0f); // cell (1,0)
        
        auto result = grid.QueryRadius(1050.0f, 1000.0f, 100.0f);
        CHECK(result.size() == 2);
        
        bool found4 = std::find(result.begin(), result.end(), id4) != result.end();
        bool found5 = std::find(result.begin(), result.end(), id5) != result.end();
        CHECK(found4);
        CHECK(found5);
    }

    SUBCASE("QueryNearest Nearest Entity Search") {
        EterBase::EntityId id10{10};
        EterBase::EntityId id20{20};
        EterBase::EntityId id30{30};

        grid.Insert(id10, 100.0f, 100.0f);
        grid.Insert(id20, 120.0f, 100.0f); // distance = 20
        grid.Insert(id30, 200.0f, 100.0f); // distance = 100

        // Nearest to (100, 100) within 50.0f should be id10 (dist 0)
        auto nearest = grid.QueryNearest(100.0f, 100.0f, 50.0f);
        REQUIRE(nearest.has_value());
        CHECK(*nearest == id10);

        // Nearest ignoring id10 should be id20 (dist 20)
        nearest = grid.QueryNearest(100.0f, 100.0f, 50.0f, id10);
        REQUIRE(nearest.has_value());
        CHECK(*nearest == id20);

        // Nearest ignoring id10 with radius 10.0f should find nothing
        nearest = grid.QueryNearest(100.0f, 100.0f, 10.0f, id10);
        CHECK_FALSE(nearest.has_value());

        // Zero or negative radius
        CHECK_FALSE(grid.QueryNearest(100.0f, 100.0f, 0.0f).has_value());
        CHECK_FALSE(grid.QueryNearest(100.0f, 100.0f, -50.0f).has_value());
    }
}

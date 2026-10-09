#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/Client/World/ActorRegistry.h"

using namespace Client::World;

TEST_CASE("ActorRegistry tests") {
    ActorRegistry registry;
    
    EntityVid vid1{1};
    EntityVid vid2{2};
    
    ActorRecord record1{
        vid1,
        1, // race
        1, // type
        100.0f, 200.0f, 300.0f, 45.0f, // x, y, z, rotation
        "Actor1",
        0, // guildId
        1, // empire
        false // isDead
    };

    ActorRecord record2{
        vid2,
        2, 2, 0.0f, 0.0f, 0.0f, 0.0f, "Actor2", 0, 1, false
    };

    SUBCASE("Initial state") {
        CHECK(registry.Count() == 0);
        CHECK_FALSE(registry.GetActor(vid1).has_value());
    }
    
    SUBCASE("Registering an actor") {
        CHECK(registry.RegisterActor(record1) == true);
        CHECK(registry.Count() == 1);
        
        // Cannot register the same actor twice
        CHECK(registry.RegisterActor(record1) == false);
        CHECK(registry.Count() == 1);
    }
    
    SUBCASE("Retrieving an actor") {
        registry.RegisterActor(record1);
        auto retrieved = registry.GetActor(vid1);
        REQUIRE(retrieved.has_value());
        CHECK(retrieved->vid == vid1);
        CHECK(retrieved->name == "Actor1");
        CHECK(retrieved->x == 100.0f);
        
        CHECK_FALSE(registry.GetActor(vid2).has_value());
    }
    
    SUBCASE("Updating a position") {
        registry.RegisterActor(record1);
        CHECK(registry.UpdatePosition(vid1, 150.0f, 250.0f, 350.0f, 90.0f) == true);
        
        auto updated = registry.GetActor(vid1);
        REQUIRE(updated.has_value());
        CHECK(updated->x == 150.0f);
        CHECK(updated->y == 250.0f);
        CHECK(updated->z == 350.0f);
        CHECK(updated->rotation == 90.0f);
        
        // Update non-existent actor
        CHECK(registry.UpdatePosition(vid2, 0, 0, 0, 0) == false);
    }
    
    SUBCASE("Setting an actor to dead") {
        registry.RegisterActor(record1);
        registry.SetDead(vid1, true);
        
        auto updated = registry.GetActor(vid1);
        REQUIRE(updated.has_value());
        CHECK(updated->isDead == true);
        
        // Setting dead on non-existent actor shouldn't crash
        registry.SetDead(vid2, true);
    }
    
    SUBCASE("Unregistering an actor") {
        registry.RegisterActor(record1);
        registry.RegisterActor(record2);
        CHECK(registry.Count() == 2);
        
        CHECK(registry.UnregisterActor(vid1) == true);
        CHECK(registry.Count() == 1);
        CHECK_FALSE(registry.GetActor(vid1).has_value());
        
        // Cannot unregister already unregistered actor
        CHECK(registry.UnregisterActor(vid1) == false);
    }
    
    SUBCASE("Clearing the registry") {
        registry.RegisterActor(record1);
        registry.RegisterActor(record2);
        CHECK(registry.Count() == 2);
        
        registry.Clear();
        CHECK(registry.Count() == 0);
        CHECK_FALSE(registry.GetActor(vid1).has_value());
        CHECK_FALSE(registry.GetActor(vid2).has_value());
    }

    SUBCASE("VisitActor functional visitor lookup and HasActor") {
        registry.RegisterActor(record1);
        CHECK(registry.HasActor(vid1));
        CHECK_FALSE(registry.HasActor(vid2));

        bool visited1 = registry.VisitActor(vid1, [&](const ActorRecord& actor) {
            CHECK(actor.vid == vid1);
            CHECK(actor.name == "Actor1");
            CHECK(actor.race == 1);
        });
        CHECK(visited1);

        bool visited2 = registry.VisitActor(vid2, [](const ActorRecord&) {
            FAIL("Should not visit non-existent actor");
        });
        CHECK_FALSE(visited2);

        // Test safe in-place mutation through visitor
        bool modified = registry.ModifyActor(vid1, [](ActorRecord& actor) {
            actor.race = 99;
        });
        CHECK(modified);

        auto updated = registry.GetActor(vid1);
        REQUIRE(updated.has_value());
        CHECK(updated->race == 99);
    }

    SUBCASE("MainActor lifecycle management") {
        CHECK(registry.GetMainActorVid().value() == 0);
        CHECK_FALSE(static_cast<bool>(registry.GetMainActorVid()));

        registry.RegisterActor(record1);
        registry.RegisterActor(record2);

        registry.SetMainActorVid(vid1);
        CHECK(registry.GetMainActorVid() == vid1);
        CHECK(static_cast<bool>(registry.GetMainActorVid()));

        // Unregistering main actor resets main actor VID
        registry.UnregisterActor(vid1);
        CHECK(registry.GetMainActorVid().value() == 0);

        registry.SetMainActorVid(vid2);
        CHECK(registry.GetMainActorVid() == vid2);

        registry.Clear();
        CHECK(registry.GetMainActorVid().value() == 0);
    }

    SUBCASE("Actor lifecycle IsAlive and IsDead") {
        // Non-existent actor
        CHECK_FALSE(registry.IsAlive(vid1));
        CHECK_FALSE(registry.IsDead(vid1));

        registry.RegisterActor(record1);
        CHECK(registry.IsAlive(vid1) == true);
        CHECK(registry.IsDead(vid1) == false);

        registry.SetDead(vid1, true);
        CHECK(registry.IsAlive(vid1) == false);
        CHECK(registry.IsDead(vid1) == true);

        registry.SetDead(vid1, false);
        CHECK(registry.IsAlive(vid1) == true);
        CHECK(registry.IsDead(vid1) == false);
    }
}

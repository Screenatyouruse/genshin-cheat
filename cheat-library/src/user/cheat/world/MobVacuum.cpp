#include "pch-il2cpp.h"
#include "MobVacuum.h"

#include <helpers.h>
#include <cheat/events.h>
#include <cheat/game/EntityManager.h>
#include <cheat/game/util.h>
#include <cheat/game/filters.h>

namespace cheat::feature 
{
    MobVacuum::MobVacuum() : Feature(),
        NF(f_Enabled,        "Mob vacuum", "MobVacuum", false),
        NF(f_IncludeAnimals, "Include living", "MobVacuum", false),
        NF(f_Speed,      "Speed",         "MobVacuum", 2.5f),
        NF(f_Distance,   "Distance",      "MobVacuum", 1.5f),
        NF(f_Radius,     "Radius",        "MobVacuum", 10.0f),
        NF(f_OnlyTarget, "Only targeted", "MobVacuum", true),
        NF(f_Instantly,  "Instantly",     "MobVacuum", false)
    {
        events::GameUpdateEvent += MY_METHOD_HANDLER(MobVacuum::OnGameUpdate);
        events::MoveSyncEvent += MY_METHOD_HANDLER(MobVacuum::OnMoveSync);
    }

    const FeatureGUIInfo& MobVacuum::GetGUIInfo() const
    {
        static const FeatureGUIInfo info{ "Mob vacuum", "World", true };
        return info;
    }

    void MobVacuum::DrawMain()
    {
        ConfigWidget("Enabled", f_Enabled, "Enables mob vacuum.\n" \
            "Mobs within the specified radius will move\nto a specified distance in front of the player.");
        ImGui::Indent();
        ConfigWidget("Include Animals", f_IncludeAnimals, "Include animals in vacuum.");
        ImGui::Unindent();
        ConfigWidget("Instant Vacuum", f_Instantly, "Vacuum entities instantly.");
        ConfigWidget("Only Hostile/Aggro", f_OnlyTarget, "If enabled, vacuum will only affect monsters targeting you. Will not affect animals.");
        ConfigWidget("Speed", f_Speed, 0.1f, 1.0f, 15.0f, "If 'Instant Vacuum' not checked, mob will be vacuumed at the specified speed.");
        ConfigWidget("Radius (m)", f_Radius, 0.1f, 5.0f, 150.0f, "Radius of vacuum.");
        ConfigWidget("Distance (m)", f_Distance, 0.1f, 0.5f, 10.0f, "Distance between the player and the monster.");
    }

    bool MobVacuum::NeedStatusDraw() const
{
        return f_Enabled.value();
    }

    void MobVacuum::DrawStatus() 
    { 
        ImGui::Text("Vacuum [%s]\n[%s|%.01fm|%.01fm|%s]", 
            f_IncludeAnimals ? "Living" : "Monsters",
            f_Instantly ? "Instant" : fmt::format("Normal|{:.1f}", f_Speed.value()).c_str(),
            f_Radius.value(),
            f_Distance.value(),
            f_OnlyTarget ? "Aggro" : "All"
        );
    }

    MobVacuum& MobVacuum::GetInstance()
    {
        static MobVacuum instance;
        return instance;
    }

    // Check if entity valid for mob vacuum.
    bool MobVacuum::IsEntityForVac(game::Entity* entity)
    {
        const auto filter = f_IncludeAnimals ?
            (game::IEntityFilter*)&game::filters::combined::Living :
            &game::filters::combined::Monsters;

        if (!filter->IsValid(entity))
            return false;

        auto& manager = game::EntityManager::instance();
        //bool isMonster = game::filters::combined::Monsters.IsValid(entity);
        if (f_OnlyTarget && game::filters::combined::Monsters.IsValid(entity))
        {
            auto monsterCombat = entity->combat();
            if (monsterCombat == nullptr || monsterCombat->fields._attackTarget.runtimeID != manager.avatar()->runtimeID())
                return false;
        }

		auto distance = manager.avatar()->distance(entity);
        return distance <= f_Radius;
    }

    // Calculate mob vacuum target position.
    app::Vector3 MobVacuum::CalcMobVacTargetPos()
    {
        auto& manager = game::EntityManager::instance();
        auto avatarEntity = manager.avatar();
        if (avatarEntity == nullptr)
            return {};

        return avatarEntity->relativePosition() + avatarEntity->forward() * f_Distance;
    }

    // Mob vacuum update function.
    // Changes position of monster, if mob vacuum enabled.
    void MobVacuum::OnGameUpdate()
    {
        static auto positions = new std::map<uint32_t, app::Vector3>();

        if (!f_Enabled.value())
            return;

        app::Vector3 targetPos = CalcMobVacTargetPos();
        if (IsVectorZero(targetPos))
            return;

        auto& manager = game::EntityManager::instance();
        auto newPositions = new std::map<uint32_t, app::Vector3>();
        for (const auto& entity : manager.entities())
        {
            if (!IsEntityForVac(entity))
                continue;

            if (f_Instantly)
            {
                entity->setRelativePosition(targetPos);
                continue;
            }

            uint32_t entityId = entity->runtimeID();
            app::Vector3 entityRelPos = positions->count(entityId) ? (*positions)[entityId] : entity->relativePosition();
            app::Vector3 newPosition = {};
            if (app::Vector3_Distance(nullptr, entityRelPos, targetPos, nullptr) < 0.1)
            {
                newPosition = targetPos;
            }
            else
            {
                app::Vector3 dir = GetVectorDirection(entityRelPos, targetPos);
                float deltaTime = app::Time_get_deltaTime(nullptr, nullptr);
                newPosition = entityRelPos + dir * f_Speed * deltaTime;
            }

            (*newPositions)[entityId] = newPosition;
            entity->setRelativePosition(newPosition);
        }

        delete positions;
        positions = newPositions;
    }

    // Mob vacuum sync packet replace.
    // Replacing move sync speed and motion state.
    //   Callow: I think it is more safe method, 
    //           because for server monster don't change position instantly.
    void MobVacuum::OnMoveSync(uint32_t entityId, app::MotionInfo* syncInfo)
    {
        if (!f_Enabled || f_Instantly)
            return;

        auto& manager = game::EntityManager::instance();
        auto entity = manager.entity(entityId);
        if (!IsEntityForVac(entity))
            return;

        app::Vector3 targetPos = CalcMobVacTargetPos();
        app::Vector3 entityPos = entity->relativePosition();
        if (app::Vector3_Distance(nullptr, targetPos, entityPos, nullptr) < 0.2)
            return;

        app::Vector3 dir = GetVectorDirection(targetPos, entityPos);
        app::Vector3 scaledDir = dir * f_Speed;

        syncInfo->fields.speed_->fields.x = scaledDir.x;
        syncInfo->fields.speed_->fields.y = scaledDir.y;
        syncInfo->fields.speed_->fields.z = scaledDir.z;

        switch (syncInfo->fields.motionState)
        {
        case app::MotionState__Enum::MotionStandby:
        case app::MotionState__Enum::MotionStandbyMove:
        case app::MotionState__Enum::MotionWalk:
        case app::MotionState__Enum::MotionDangerDash:
            syncInfo->fields.motionState = app::MotionState__Enum::MotionRun;
        }
    }
}


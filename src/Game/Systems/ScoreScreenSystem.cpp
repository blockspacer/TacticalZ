#include "Game/Systems/ScoreScreenSystem.h"


ScoreScreenSystem::ScoreScreenSystem(SystemParams params)
    : System(params)
    , PureSystem("ScoreScreen")
{
    EVENT_SUBSCRIBE_MEMBER(m_EPlayerDeath, &ScoreScreenSystem::OnPlayerDeath);
    EVENT_SUBSCRIBE_MEMBER(m_EPlayerSpawned, &ScoreScreenSystem::OnPlayerSpawn);
    EVENT_SUBSCRIBE_MEMBER(m_EPlayerConnected, &ScoreScreenSystem::OnPlayerConnected);
    EVENT_SUBSCRIBE_MEMBER(m_EPlayerDisconnected, &ScoreScreenSystem::OnPlayerDisconnected);
}

void ScoreScreenSystem::UpdateComponent(EntityWrapper& entity, ComponentWrapper& capturePoint, double dt)
{
    if (!IsServer) {
        return;
    }

    if(!entity.HasComponent("ScoreScreen")){
        return;
    }

    int redTeamEnum;
    int blueTeamEnum;
    int spectatorTeamEnum;
    int currentTeam = 0;

    if(entity.HasComponent("Team")) {
        auto cTeam = entity["Team"];
        redTeamEnum = (int)cTeam["Team"].Enum("Red");
        blueTeamEnum = (int)cTeam["Team"].Enum("Blue");
        spectatorTeamEnum = (int)cTeam["Team"].Enum("Spectator");
        currentTeam = (int)cTeam["Team"];
    }

    auto children = entity.ChildrenWithComponent("ScoreIdentity");

    float position = 0.f;

    for (auto it = m_PlayerIdentities.begin(); it != m_PlayerIdentities.end(); ++it) {

        bool found = false;
        for (auto child : children) {
            int ID = (int)child["ScoreIdentity"]["ID"];
            if (it->first == ID) {
                if (it->second.Team != currentTeam) {
                    m_World->DeleteEntity(child.ID);
                    (int&)entity["ScoreScreen"]["TotalIdentities"] -= 1;
                    break;
                }
                for (auto it = m_DisconnectedIdentities.begin(); it != m_DisconnectedIdentities.end(); ++it) {
                    if (ID = *it) {

                        m_World->DeleteEntity(child.ID);
                        (int&)entity["ScoreScreen"]["TotalIdentities"] -= 1;
                        it = m_DisconnectedIdentities.erase(it);
                        break;
                    }
                }
                found = true;
                //Update position for childs
                glm::vec3 offset = (glm::vec3)entity["ScoreScreen"]["Offset"];
                (glm::vec3&) child["Transform"]["Position"] = offset * position;
                position += 1.f;

                break;
            }
        }
        if(found == false) {
            if(it->second.Team != currentTeam) {
                //This player is not the same team as this scoreboard should show.
                continue;
            }
            //There is no entry for this player, create one.
            auto entityFile = ResourceManager::Load<EntityFile>("Schema/Entities/ScoreIdentity.xml");
            EntityWrapper scoreIdentity = entityFile->MergeInto(m_World);
            glm::vec3 offset = (glm::vec3)entity["ScoreScreen"]["Offset"];
            int newPosition = (int)entity["ScoreScreen"]["TotalIdentities"];
            (glm::vec3&) scoreIdentity["Transform"]["Position"] = offset * (float)newPosition;
            auto cScoreIdentity = scoreIdentity["ScoreIdentity"];
            auto data = it->second;

            (std::string&)cScoreIdentity["Name"] = data.Name;
            (int&)cScoreIdentity["ID"] = data.ID;
            (int&)cScoreIdentity["Ping"] = 1337;

            m_World->SetParent(scoreIdentity.ID, entity.ID);

            (int&)entity["ScoreScreen"]["TotalIdentities"] += 1;

        }
    }
    //Local player should have an icon, compare with LocalPlayer somthing
}

bool ScoreScreenSystem::OnPlayerDeath(const Events::PlayerDeath& e)
{
    //When player die, add it to his score, and when possible the player who killed him.
    return 0;
}

bool ScoreScreenSystem::OnPlayerSpawn(const Events::PlayerSpawned& e)
{
    //When a player spawn, add data to the list entry with the same ID.

    std::unordered_map<int, PlayerData>::iterator got;

    got = m_PlayerIdentities.find(e.PlayerID);
    if(got == m_PlayerIdentities.end()) {
        LOG_ERROR("Player spawned without having an entry on score screen");
        return 0;
    }
    auto& data = got->second;
    data.Player = e.Player;
    data.Team = (int)data.Player["Team"]["Team"];

    return 0;
}

bool ScoreScreenSystem::OnPlayerConnected(const Events::PlayerConnected& e)
{
    //player has connected, add his data to the list of ScoreIdentities
    PlayerData data;
    data.ID = e.PlayerID;
    data.Name = e.PlayerName;
    m_PlayerIdentities.insert( {data.ID, data} );

    return 0;
}

bool ScoreScreenSystem::OnPlayerDisconnected(const Events::PlayerDisconnected& e)
{
    //player has disconnected, remove him from list of ScoreIdentities
    m_DisconnectedIdentities.push_back(e.PlayerID);
    m_PlayerIdentities.erase(e.PlayerID);
    return 0;
}

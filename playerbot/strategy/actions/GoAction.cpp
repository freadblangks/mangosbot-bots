#include "botpch.h"
#include "../../playerbot.h"
#include "GoAction.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"
#include "../values/Formations.h"
#include "../values/PositionValue.h"

using namespace ai;

vector<string> split(const string &s, char delim);
char *strstri(const char *haystack, const char *needle);

bool GoAction::Execute(Event event)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    string param = event.getParam();
    if (param == "?")
    {
        float x = bot->GetPositionX();
        float y = bot->GetPositionY();
        Map2ZoneCoordinates(x, y, bot->GetZoneId());
        ostringstream out;
        out << "I am at " << x << "," << y;
        ai->TellMaster(out.str());
        return true;
    }

    list<ObjectGuid> gos = ChatHelper::parseGameobjects(param);
    if (!gos.empty())
    {
        for (list<ObjectGuid>::iterator i = gos.begin(); i != gos.end(); ++i)
        {
            GameObject* go = ai->GetGameObject(*i);
            if (go && sServerFacade.isSpawned(go))
            {
                if (sServerFacade.IsDistanceGreaterThan(sServerFacade.GetDistance2d(bot, go), sPlayerbotAIConfig.reactDistance))
                {
                    ai->TellError("It is too far away");
                    return false;
                }

                ostringstream out; out << "Moving to " << ChatHelper::formatGameobject(go);
                ai->TellMasterNoFacing(out.str());
                return MoveNear(bot->GetMapId(), go->GetPositionX(), go->GetPositionY(), go->GetPositionZ() + 0.5f, sPlayerbotAIConfig.followDistance);
            }
        }
        return false;
    }

    list<ObjectGuid> units;
    list<ObjectGuid> npcs = AI_VALUE(list<ObjectGuid>, "nearest npcs");
    units.insert(units.end(), npcs.begin(), npcs.end());
    list<ObjectGuid> players = AI_VALUE(list<ObjectGuid>, "nearest friendly players");
    units.insert(units.end(), players.begin(), players.end());
    for (list<ObjectGuid>::iterator i = units.begin(); i != units.end(); i++)
    {
        Unit* unit = ai->GetUnit(*i);
        if (unit && strstri(unit->GetName(), param.c_str()))
        {
            ostringstream out; out << "Moving to " << unit->GetName();
            ai->TellMasterNoFacing(out.str());
            return MoveNear(bot->GetMapId(), unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ() + 0.5f, sPlayerbotAIConfig.followDistance);
        }
    }

    if (param.find(";") != string::npos)
    {
        vector<string> coords = split(param, ';');
        float x = atof(coords[0].c_str());
        float y = atof(coords[1].c_str());
        float z;
        if (coords.size() > 2)
            z = atof(coords[2].c_str());
        else
            z = bot->GetPositionZ();

        if (bot->IsWithinLOS(x, y, z))
            return MoveNear(bot->GetMapId(), x, y, z, 0);
        else
            return MoveTo(bot->GetMapId(), x, y, z, false, false);

        /*
        if (bot->IsSitState())
            bot->SetStandState(UNIT_STAND_STATE_STAND);

        if (bot->IsNonMeleeSpellCasted(true))
        {
            bot->CastStop();
            ai->InterruptSpell();
        }

        MotionMaster& mm = *bot->GetMotionMaster();

        bool generatePath = !bot->IsFlying() && !sServerFacade.IsUnderwater(bot);

#ifdef MANGOS
        mm.MovePoint(mapId, x, y, z, generatePath);
#endif
#ifdef CMANGOS
        if (ai->HasStrategy("debug", BOT_STATE_NON_COMBAT))
        {
            ai->TellMaster("Point Traveling");

            ostringstream out;
            out << "From: " << bot->GetPositionX() << " | " << bot->GetPositionY() << " | " << bot->GetPositionZ();
            out << " to: " << x << " | " << y << " | " << z;
            ai->TellMaster(out);
        }
        bot->StopMoving();
        mm.Clear();
        mm.MovePoint(bot->GetMapId(), x, y, z, FORCED_MOVEMENT_RUN, generatePath);
#endif
*/

        return true;
    }


    if (param.find(",") != string::npos)
    {
        vector<string> coords = split(param, ',');
        float x = atof(coords[0].c_str());
        float y = atof(coords[1].c_str());
        Zone2MapCoordinates(x, y, bot->GetZoneId());

        Map* map = bot->GetMap();
        float z = bot->GetPositionZ();
        bot->UpdateAllowedPositionZ(x, y, z);

        if (sServerFacade.IsDistanceGreaterThan(sServerFacade.GetDistance2d(bot, x, y), sPlayerbotAIConfig.reactDistance))
        {
            ai->TellMaster("It is too far away");
            return false;
        }

        const TerrainInfo* terrain = map->GetTerrain();
        if (terrain->IsUnderWater(x, y, z) || terrain->IsInWater(x, y, z))
        {
            ai->TellError("It is under water");
            return false;
        }

#ifdef MANGOSBOT_TWO
        float ground = map->GetHeight(bot->GetPhaseMask(), x, y, z + 0.5f);
#else
        float ground = map->GetHeight(x, y, z + 0.5f);
#endif
        if (ground <= INVALID_HEIGHT)
        {
            ai->TellError("I can't go there");
            return false;
        }

        float x1 = x, y1 = y;
        Map2ZoneCoordinates(x1, y1, bot->GetZoneId());
        ostringstream out; out << "Moving to " << x1 << "," << y1;
        ai->TellMasterNoFacing(out.str());
        return MoveNear(bot->GetMapId(), x, y, z + 0.5f, sPlayerbotAIConfig.followDistance);
    }

    ai::PositionEntry pos = context->GetValue<ai::PositionMap&>("position")->Get()[param];
    if (pos.isSet())
    {
        if (sServerFacade.IsDistanceGreaterThan(sServerFacade.GetDistance2d(bot, pos.x, pos.y), sPlayerbotAIConfig.reactDistance))
        {
            ai->TellError("It is too far away");
            return false;
        }

        ostringstream out; out << "Moving to position " << param;
        ai->TellMasterNoFacing(out.str());
        return MoveNear(bot->GetMapId(), pos.x, pos.y, pos.z + 0.5f, sPlayerbotAIConfig.followDistance);
    }

    ai->TellMaster("Whisper 'go x,y', 'go [game object]', 'go unit' or 'go position' and I will go there");
    return false;
}

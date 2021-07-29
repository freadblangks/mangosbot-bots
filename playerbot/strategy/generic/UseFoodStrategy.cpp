#include "botpch.h"
#include "../../playerbot.h"
#include "UseFoodStrategy.h"

using namespace ai;

void UseFoodStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    Strategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "low health",
        NextAction::array(0, new NextAction("food", 3.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium mana",
        NextAction::array(0, new NextAction("drink", 3.0f), NULL)));
}

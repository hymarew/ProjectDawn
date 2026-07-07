#include "gameContext.h"

GameContext& GameContext::Instance()
{
    static GameContext s_Instance;
    return s_Instance;
}

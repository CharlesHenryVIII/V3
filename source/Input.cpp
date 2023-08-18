#include "Input.h"
#include "Rendering.h"

void CommandHandler::InputUpdate()
{
    for (auto& key : keyStates)
    {
        if (key.second.down)
        {
            key.second.upThisFrame = false;
            if (key.second.downPrevFrame)
            {
                key.second.downThisFrame = false;
            }
            else
            {
                key.second.downThisFrame = true;
            }
        }
        else
        {
            key.second.downThisFrame = false;
            if (key.second.downPrevFrame)
            {
                key.second.upThisFrame = true;
            }
            else
            {
                key.second.upThisFrame = false;
            }
        }
        key.second.downPrevFrame = key.second.down;
    }

    if (mouse.wheelModifiedLastFrame)
    {
        mouse.wheelInstant.y = 0;
        mouse.wheelModifiedLastFrame = false;
    }
    else if (mouse.wheelInstant.y)
    {
        mouse.wheelModifiedLastFrame = true;
    }

    //mouse.pDelta *= m_sensitivity;
}


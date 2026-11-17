// Crash handler dla platform bez implementacji - pozwala skompilować projekt poza Windows
#if !defined(_WIN32)

#include "engine/CrashHandler.hpp"

#include "engine/Logger.hpp"

namespace Engine
{
    CCrashHandler::CCrashHandler()
    {
        LOG_WARN("Crash handler nie jest obslugiwany na tej platformie - zrzuty pamieci nie beda zapisywane");
    }

    CCrashHandler::~CCrashHandler() = default;

    bool CCrashHandler::IsSupported()
    {
        return false;
    }
}

#endif

#include <cstdio>
#include <cstdarg>

#include "common.hpp"
#include "gui.hpp"
#include "menus.hpp"

static FILE *g_log = nullptr;

static void Log(const char *fmt, ...) {
    if (!g_log) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    std::vfprintf(g_log, fmt, args);
    va_end(args);
    std::fputc('\n', g_log);
    std::fflush(g_log);
}

#define STEP(name, expr) \
    do { \
        Log("start %s", name); \
        Result step_ret = (expr); \
        Log("done  %s -> 0x%x", name, step_ret); \
    } while (0)

namespace Services {
    void Exit(void) {
        Log("exit begin");
        GUI::Exit();
        romfsExit();
        Log("exit end");
    }

    void Init(void) {
        STEP("romfsInit", romfsInit());
        STEP("setInitialize", setInitialize());
        STEP("setsysInitialize", setsysInitialize());
        STEP("setcalInitialize", setcalInitialize());
        STEP("splInitialize", splInitialize());
        STEP("nifmInitialize", nifmInitialize(NifmServiceType_User));
        STEP("appletInitialize", appletInitialize());
        STEP("apmInitialize", apmInitialize());
        STEP("nsInitialize", nsInitialize());
        STEP("psmInitialize", psmInitialize());

        if (hosversionAtLeast(8, 0, 0)) {
            STEP("clkrstInitialize", clkrstInitialize());
        }
        else {
            STEP("pcvInitialize", pcvInitialize());
        }

        STEP("tsInitialize", tsInitialize());
        STEP("hiddbgInitialize", hiddbgInitialize());
        STEP("hidsysInitialize", hidsysInitialize());
        STEP("gpioInitialize", gpioInitialize());
        STEP("gpioOpenSession", gpioOpenSession(&hp_inserted, static_cast<GpioPadName>(21)));

        Log("start GUI::Init");
        int gui_ret = GUI::Init();
        Log("done  GUI::Init -> %d", gui_ret);
    }
}

int main(int argc, char **argv) {
    g_log = std::fopen("sdmc:/OunceIdent.log", "w");
    Log("main reached, hos version %u.%u.%u", HOSVER_MAJOR(hosversionGet()), HOSVER_MINOR(hosversionGet()), HOSVER_MICRO(hosversionGet()));
    Services::Init();
    Log("start Menus::Main");
    Menus::Main();
    Services::Exit();
    return 0;
}

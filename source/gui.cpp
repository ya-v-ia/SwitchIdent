#include <cstdio>
#include <cstdarg>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>

#include "gui.hpp"

void OunceLog(const char *fmt, ...);

SDL_Texture *banner = nullptr, *drive = nullptr, *menu_icons[7] = { 0 };

namespace GUI {
    static SDL_Window *g_window = nullptr;
    static SDL_Renderer *g_renderer = nullptr;
    static TTF_Font *g_font = nullptr;
    static int g_dbg_text_calls = 0;
    static int g_dbg_fail_calls = 0;

    static void LoadImage(SDL_Texture **texture, const char *path) {
        SDL_Surface *image = IMG_Load(path);

        if (!image) {
            OunceLog("IMG_Load(%s) failed: %s", path, IMG_GetError());
            return;
        }

        *texture = SDL_CreateTextureFromSurface(g_renderer, image);
        SDL_FreeSurface(image);
    }

    static void PrewarmGlyphs(void) {
        SDL_Color white = { 255, 255, 255, 255 };
        int failed = 0;
        char failed_list[64] = { 0 };

        for (int c = 33; c <= 126; c++) {
            char s[2] = { static_cast<char>(c), 0 };
            SDL_Surface *surf = TTF_RenderUTF8_Blended(g_font, s, white);
            if (surf) {
                SDL_FreeSurface(surf);
            }
            else {
                if (failed < 60) {
                    failed_list[failed] = static_cast<char>(c);
                }
                failed++;
                if (failed <= 3) {
                    OunceLog("prewarm: char '%c' failed: %s", c, TTF_GetError());
                }
            }
        }

        SDL_Surface *deg = TTF_RenderUTF8_Blended(g_font, "\xC2\xB0", white);
        if (deg) {
            SDL_FreeSurface(deg);
        }
        else {
            failed++;
            OunceLog("prewarm: degree sign failed: %s", TTF_GetError());
        }

        OunceLog("prewarm done: %d failed, first failures: [%s]", failed, failed_list);
    }

    static void TestRender(const char *label, SDL_Surface *s) {
        if (s) {
            OunceLog("%s: ok %dx%d format=0x%x", label, s->w, s->h, (unsigned) s->format->format);
            SDL_FreeSurface(s);
        }
        else {
            OunceLog("%s: NULL, TTF_GetError=\"%s\" SDL_GetError=\"%s\"", label, TTF_GetError(), SDL_GetError());
        }
    }

    int Init(void) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            OunceLog("SDL_Init failed: %s", SDL_GetError());
            return -1;
        }

        g_window = SDL_CreateWindow("OunceIdent", 0, 0, 1920, 1080, SDL_WINDOW_FULLSCREEN);
        if (g_window == nullptr) {
            OunceLog("SDL_CreateWindow failed: %s", SDL_GetError());
            return -2;
        }

        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
        if (g_renderer == nullptr) {
            OunceLog("SDL_CreateRenderer failed: %s", SDL_GetError());
            return -3;
        }

        int flags = IMG_INIT_PNG;
        if ((IMG_Init(flags) & flags) != flags) {
            OunceLog("IMG_Init failed: %s", IMG_GetError());
            return -4;
        }

        GUI::LoadImage(&banner, "romfs:/banner.png");
        GUI::LoadImage(&drive, "romfs:/drive.png");
        GUI::LoadImage(&menu_icons[0], "romfs:/kernel.png");
        GUI::LoadImage(&menu_icons[1], "romfs:/system.png");
        GUI::LoadImage(&menu_icons[2], "romfs:/power.png");
        GUI::LoadImage(&menu_icons[3], "romfs:/storage.png");
        GUI::LoadImage(&menu_icons[4], "romfs:/joycon.png");
        GUI::LoadImage(&menu_icons[5], "romfs:/misc.png");
        GUI::LoadImage(&menu_icons[6], "romfs:/exit.png");

        SDL_version sdl_v;
        SDL_GetVersion(&sdl_v);
        const SDL_version *ttf_v = TTF_Linked_Version();
        OunceLog("SDL %d.%d.%d, SDL_ttf %d.%d.%d, renderer driver count=%d", sdl_v.major, sdl_v.minor, sdl_v.patch, ttf_v->major, ttf_v->minor, ttf_v->patch, SDL_GetNumRenderDrivers());

        SDL_RendererInfo info;
        if (SDL_GetRendererInfo(g_renderer, &info) == 0) {
            OunceLog("renderer: %s flags=0x%x", info.name, info.flags);
        }

        if (TTF_Init() != 0) {
            OunceLog("TTF_Init failed: %s", TTF_GetError());
            return -5;
        }
        g_font = TTF_OpenFont("romfs:/Ubuntu-Regular.ttf", 36);
        if (g_font == nullptr) {
            OunceLog("TTF_OpenFont failed: %s", TTF_GetError());
            return -6;
        }

        int tw = 0, th = 0;
        int size_ret = TTF_SizeText(g_font, "Kernel", &tw, &th);
        OunceLog("TTF_SizeText(Kernel) ret=%d size=%dx%d fontHeight=%d glyphK=%d", size_ret, tw, th, TTF_FontHeight(g_font), TTF_GlyphIsProvided(g_font, 'K'));

        SDL_Color white = { 255, 255, 255, 255 };
        TestRender("init Blended", TTF_RenderUTF8_Blended(g_font, "Kernel", white));

        PrewarmGlyphs();

        TestRender("post-prewarm Blended 'SwitchIdent v0.5'", TTF_RenderUTF8_Blended(g_font, "SwitchIdent v0.5", white));
        return 0;
    }

    void Exit(void) {
        if (g_font) TTF_CloseFont(g_font);
        for (int i = 6; i >= 0; i--) {
            if (menu_icons[i]) SDL_DestroyTexture(menu_icons[i]);
        }
        if (banner) SDL_DestroyTexture(banner);
        if (drive) SDL_DestroyTexture(drive);
        TTF_Quit();
        IMG_Quit();
        if (g_renderer) SDL_DestroyRenderer(g_renderer);
        if (g_window) SDL_DestroyWindow(g_window);
        SDL_Quit();
    }

    void ClearScreen(SDL_Color colour) {
        if (!g_renderer) return;
        SDL_SetRenderDrawColor(g_renderer, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderClear(g_renderer);
    }

    void DrawRect(int x, int y, int w, int h, SDL_Color colour) {
        if (!g_renderer) return;
        SDL_Rect rect;
        rect.x = x; rect.y = y; rect.w = w; rect.h = h;
        SDL_SetRenderDrawColor(g_renderer, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderFillRect(g_renderer, &rect);
    }

    static void DrawSurface(SDL_Surface *surface, int x, int y) {
        SDL_Texture *texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        if (!texture) {
            if (g_dbg_fail_calls++ < 10) OunceLog("SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
            return;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_Rect position = {x, y, surface->w, surface->h};
        if (SDL_RenderCopy(g_renderer, texture, nullptr, &position) != 0) {
            if (g_dbg_fail_calls++ < 10) OunceLog("SDL_RenderCopy failed: %s", SDL_GetError());
        }
        SDL_DestroyTexture(texture);
    }

    void DrawText(int x, int y, int size, SDL_Color colour, const char *text) {
        if (!g_renderer || !g_font) {
            if (g_dbg_fail_calls++ < 10) OunceLog("DrawText: renderer or font is null");
            return;
        }

        colour.a = 255;

        SDL_Surface *raw = TTF_RenderUTF8_Blended(g_font, text, colour);
        if (!raw) {
            if (g_dbg_fail_calls++ < 10) OunceLog("DrawText(\"%s\"): Blended returned NULL: %s", text, TTF_GetError());
            return;
        }

        SDL_Surface *surface = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(raw);
        if (!surface) {
            if (g_dbg_fail_calls++ < 10) OunceLog("DrawText(\"%s\"): convert returned NULL: %s", text, SDL_GetError());
            return;
        }

        if (g_dbg_text_calls < 8) {
            g_dbg_text_calls++;
            OunceLog("DrawText \"%s\" at %d,%d surface %dx%d", text, x, y, surface->w, surface->h);
        }

        DrawSurface(surface, x, y);

        SDL_SetRenderDrawColor(g_renderer, 255, 0, 255, 255);
        SDL_Rect box = {x, y, surface->w, surface->h};
        SDL_RenderDrawRect(g_renderer, &box);

        SDL_FreeSurface(surface);
    }

    void DrawTextf(int x, int y, int size, SDL_Color colour, const char* text, ...) {
        char buffer[256];
        va_list args;
        va_start(args, text);
        std::vsnprintf(buffer, 256, text, args);
        GUI::DrawText(x, y, size, colour, buffer);
        va_end(args);
    }

    void GetTextDimensions(int size, const char *text, int *width, int *height) {
        int w = 0, h = 0;
        if (g_font) {
            TTF_SizeText(g_font, text, &w, &h);
        }
        if (width) *width = w;
        if (height) *height = h;
    }

    void DrawImage(SDL_Texture *texture, int x, int y) {
        if (!g_renderer || !texture) return;
        SDL_Rect position;
        position.x = x; position.y = y;
        SDL_QueryTexture(texture, nullptr, nullptr, &position.w, &position.h);
        SDL_RenderCopy(g_renderer, texture, nullptr, &position);
    }

    void Render(void) {
        if (!g_renderer) return;

        SDL_SetRenderDrawColor(g_renderer, 255, 0, 255, 255);
        SDL_Rect marker = {1700, 1000, 100, 40};
        SDL_RenderFillRect(g_renderer, &marker);

        SDL_RenderPresent(g_renderer);
    }
}

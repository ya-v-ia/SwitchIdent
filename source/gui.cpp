#include <cstdio>
#include <cstdarg>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>

#include "gui.hpp"

SDL_Texture *banner = nullptr, *drive = nullptr, *menu_icons[7] = { 0 };

namespace GUI {
    static SDL_Window *g_window = nullptr;
    static SDL_Renderer *g_renderer = nullptr;
    static TTF_Font *g_font = nullptr;
    static int g_dbg_lines = 0;
    static int g_dbg_text_calls = 0;

    static void DbgLog(const char *fmt, ...) {
        if (g_dbg_lines >= 40) {
            return;
        }
        g_dbg_lines++;
        FILE *f = std::fopen("sdmc:/OunceIdent.log", "a");
        if (!f) {
            return;
        }
        va_list args;
        va_start(args, fmt);
        std::vfprintf(f, fmt, args);
        va_end(args);
        std::fputc('\n', f);
        std::fclose(f);
    }

    static void LoadImage(SDL_Texture **texture, const char *path) {
        SDL_Surface *image = IMG_Load(path);

        if (!image) {
            SDL_Log("IMG_Load failed: %s\n", IMG_GetError());
            return;
        }

        *texture = SDL_CreateTextureFromSurface(g_renderer, image);
        SDL_FreeSurface(image);
    }

    int Init(void) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
            return -1;
        }

        g_window = SDL_CreateWindow("OunceIdent", 0, 0, 1920, 1080, SDL_WINDOW_FULLSCREEN);
        if (g_window == nullptr) {
            SDL_Log("SDL_CreateWindow failed: %s\n", SDL_GetError());
            return -2;
        }

        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
        if (g_renderer == nullptr) {
            SDL_Log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
            return -3;
        }

        int flags = IMG_INIT_PNG;
        if ((IMG_Init(flags) & flags) != flags) {
            SDL_Log("IMG_Init failed: %s\n", IMG_GetError());
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

        if (TTF_Init() != 0) {
            SDL_Log("TTF_Init failed: %s\n", TTF_GetError());
            return -5;
        }
        g_font = TTF_OpenFont("romfs:/Ubuntu-Regular.ttf", 36);
        if (g_font == nullptr) {
            SDL_Log("TTF_OpenFont failed: %s\n", TTF_GetError());
            return -6;
        }

        int tw = 0, th = 0;
        TTF_SizeText(g_font, "Kernel", &tw, &th);
        DbgLog("font ok, TTF_SizeText(\"Kernel\") = %d x %d, height=%d, glyph 'K' provided=%d", tw, th, TTF_FontHeight(g_font), TTF_GlyphIsProvided(g_font, 'K'));
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
            DbgLog("SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
            return;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_Rect position = {x, y, surface->w, surface->h};
        if (SDL_RenderCopy(g_renderer, texture, nullptr, &position) != 0) {
            DbgLog("SDL_RenderCopy failed: %s", SDL_GetError());
        }
        SDL_DestroyTexture(texture);
    }

    void DrawText(int x, int y, int size, SDL_Color colour, const char *text) {
        if (!g_renderer || !g_font) {
            DbgLog("DrawText: renderer or font is null");
            return;
        }

        colour.a = 255;

        SDL_Surface *raw = TTF_RenderUTF8_Blended(g_font, text, colour);
        if (!raw) {
            DbgLog("DrawText: TTF_RenderUTF8_Blended returned null: %s", TTF_GetError());
            return;
        }

        SDL_Surface *surface = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(raw);
        if (!surface) {
            DbgLog("DrawText: SDL_ConvertSurfaceFormat returned null: %s", SDL_GetError());
            return;
        }

        if (g_dbg_text_calls < 6) {
            g_dbg_text_calls++;
            int nonzero = 0;
            int max_alpha = 0;
            for (int row = 0; row < surface->h; row++) {
                const Uint8 *p = static_cast<const Uint8 *>(surface->pixels) + row * surface->pitch;
                for (int col = 0; col < surface->w; col++) {
                    int a = p[col * 4 + 3];
                    if (a > 0) nonzero++;
                    if (a > max_alpha) max_alpha = a;
                }
            }
            DbgLog("DrawText \"%s\" at %d,%d surface %dx%d nonzero_alpha_pixels=%d max_alpha=%d colour=%d,%d,%d", text, x, y, surface->w, surface->h, nonzero, max_alpha, colour.r, colour.g, colour.b);
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

        if (g_font) {
            SDL_Color white = { 255, 255, 255, 255 };
            SDL_Surface *solid = TTF_RenderUTF8_Solid(g_font, "SOLID TEST", white);
            if (solid) {
                DrawSurface(solid, 450, 1000);
                SDL_FreeSurface(solid);
            }
            SDL_Surface *blended = TTF_RenderUTF8_Blended(g_font, "BLENDED TEST", white);
            if (blended) {
                DrawSurface(blended, 450, 1040);
                SDL_FreeSurface(blended);
            }
        }

        SDL_RenderPresent(g_renderer);
    }
}

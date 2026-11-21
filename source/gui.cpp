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

    void DrawText(int x, int y, int size, SDL_Color colour, const char *text) {
        if (!g_renderer || !g_font) return;

        SDL_Surface *surface = TTF_RenderUTF8_Blended(g_font, text, colour);
        if (!surface) return;
        SDL_Texture *texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        if (texture) {
            SDL_Rect position = {x, y, surface->w, surface->h};
            SDL_RenderCopy(g_renderer, texture, nullptr, &position);
            SDL_DestroyTexture(texture);
        }
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
        if (!g_font) { *width = 0; *height = 0; return; }
        TTF_SizeText(g_font, text, width, height);
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
        SDL_RenderPresent(g_renderer);
    }
}

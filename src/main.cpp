/**
 *
 * Ewig - Awesome immutable text editor
 *
 * Author Juan Pedor Que no Puedor
 *
 **/

#include <immer/flex_vector.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>

namespace ewig {

int init_window()
{
    SDL_Init(SDL_INIT_VIDEO);
    if (TTF_Init() == -1) {
        std::cerr << " Failed to initialize TTF: " << SDL_GetError() << std::endl;
        return 1;
    }

    auto window = SDL_CreateWindow(
        "eWig - Maria's tOll eDitor",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        SDL_WINDOW_OPENGL);

    if (window == nullptr) {
        std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }
    auto renderer = SDL_CreateRenderer(window, -1, 0);
    auto font = TTF_OpenFont("/home/raskolnikov/.fonts/Inconsolata-Regular.ttf", 24);
    if (!font) {
        std::cerr << "Erro loading font: " << TTF_GetError() << std::endl;
        return 1;
    }

    auto color = SDL_Color{255, 255, 255};
    auto surface = TTF_RenderText_Solid(font, "Wilkommen bei Ewig", color);
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);

    auto rect = SDL_Rect{20, 20};
    SDL_QueryTexture(texture, nullptr, nullptr, &rect.w, &rect.h);

    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_RenderPresent(renderer);

    SDL_Delay(3000);

    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);

    SDL_Quit();
    return 0;
}

} // namespace ewig

int main(int argc, char* argv[])
{
    return ewig::init_window();
}

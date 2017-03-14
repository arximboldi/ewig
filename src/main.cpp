/**
 *
 * Ewig - Awesome immutable text editor
 *
 * Author Juan Pedor Que no Puedor
 *
 **/

#include <immer/flex_vector.hpp>
#include <SDL2/SDL.h>
#include <iostream>

namespace ewig {

int init_window()
{
    SDL_Init(SDL_INIT_VIDEO);

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

    SDL_Delay(3000);

    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}

} // namespace ewig

int main(int argc, char* argv[])
{
    return ewig::init_window();
}

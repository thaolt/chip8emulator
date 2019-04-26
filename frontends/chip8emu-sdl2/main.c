#include "stdint.h"
#ifdef WIN32
#include "SDL.h"
#else
#include "SDL2/SDL.h"
#endif

#include "log.h"
#include "tinycthread.h"
#include "chip8emu.h"

/* Keypad keymap */
static int keymap[16] = {
    SDLK_KP_0,
    SDLK_KP_1,
    SDLK_KP_2,
    SDLK_KP_3,
    SDLK_UP,
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_DOWN,
    SDLK_KP_8,
    SDLK_KP_9,
    SDLK_KP_ENTER,
    SDLK_KP_PERIOD,
    SDLK_KP_PLUS,
    SDLK_KP_MINUS,
    SDLK_KP_MULTIPLY,
    SDLK_KP_DIVIDE,
};

static mtx_t key_mtx;
static mtx_t draw_mtx;
static cnd_t draw_cnd;

static bool keystates[16] = {0};
static chip8emu_snapshot snapshot;

void draw_callback(chip8emu *cpu) {
    (void)cpu;
    mtx_lock(&draw_mtx);
    cnd_broadcast(&draw_cnd);
    mtx_unlock(&draw_mtx);
}

bool keystate_callback(chip8emu* cpu, uint8_t key) {
    (void)cpu;
    mtx_lock(&key_mtx);
    bool ret = keystates[key];
    mtx_unlock(&key_mtx);
    return ret;
}

void beep_callback(chip8emu * cpu) {
    (void)cpu;
}

int keypad_thread(void *arg) {
    (void)arg;

    SDL_Event e;
    bool quit = false;
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 1000000
    };
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;

            if (e.type == SDL_KEYDOWN) {
                mtx_lock(&key_mtx);
                for (int i = 0; i < 16; ++i) {
                    if (e.key.keysym.sym == keymap[i]) {
                        keystates[i] = true;
                    }
                }
                mtx_unlock(&key_mtx);
            }

            if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    quit = true;
                else {
                    mtx_lock(&key_mtx);
                    for (int i = 0; i < 16; ++i) {
                        if (e.key.keysym.sym == keymap[i]) {
                            keystates[i] = false;
                        }
                    }
                    mtx_unlock(&key_mtx);
                }
            }
        }
        thrd_sleep(&delay, 0);
    }
    return 0;
}


int display_draw_thread(void *arg) {
    chip8emu * cpu = (chip8emu*) arg;

    int w = 640;
    int h = 320;

    SDL_Window* window;
    SDL_Renderer *renderer;
    SDL_Texture* sdlTexture;

    window = NULL;

    if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        exit(1);
    }

    window = SDL_CreateWindow(
            "CHIP-8 Emulator",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            w, h, SDL_WINDOW_SHOWN
    );

    if (window == NULL){
        printf( "Window could not be created! SDL_Error: %s\n",
                SDL_GetError() );
        exit(2);
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, w, h);

    // Create texture that stores frame buffer
    sdlTexture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            64, 32);

    // Emulation loop
    void* pixels;
    int pitch;
    mtx_lock(&draw_mtx);
    while (true) {
        cnd_wait(&draw_cnd, &draw_mtx);
        chip8emu_take_snapshot(cpu, &snapshot);

        // Update SDL texture
        SDL_LockTexture(sdlTexture, NULL, &pixels, &pitch);
        for (int i = 0; i < 2048; ++i) {
            ((Uint32 *) pixels)[i] = snapshot.gfx[i] * 0x00FFFFFF | 0xFF000000;
        }
        SDL_UnlockTexture(sdlTexture);
        // Clear screen and render
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
        SDL_RenderPresent(renderer);

    }
    mtx_unlock(&draw_mtx);

}


int main(int argc, char **argv) {
    (void) argc; (void) argv;

    mtx_init(&draw_mtx, mtx_plain);
    mtx_init(&key_mtx, mtx_plain);
    cnd_init(&draw_cnd);

    chip8emu* cpu= chip8emu_new();
    cpu->draw = &draw_callback;
    cpu->keystate = &keystate_callback;
    cpu->beep = &beep_callback;

    thrd_t thrd_draw;
    thrd_t thrd_keypad;

    if (thrd_create(&thrd_keypad, keypad_thread, (void*)cpu) != thrd_success) {
        log_error("Cannot create keypad thread!");
        goto quit;
    }

    if (thrd_create(&thrd_draw, display_draw_thread, (void*)cpu) != thrd_success) {
        log_error("Cannot create draw thread!");
        goto quit;
    }

    chip8emu_load_rom(cpu, "E:\\Workspaces\\roms\\TETRIS");
    chip8emu_set_cpu_speed(cpu, 1200);
    chip8emu_start(cpu);

    thrd_join(thrd_keypad, NULL);

quit:
    return 0;


}

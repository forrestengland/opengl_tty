#include <stdio.h>

#include <SDL2/SDL.h>
#include <GLES2/gl2.h>

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr,
                "SDL_Init failed: %s\n",
                SDL_GetError());
        return 1;
    }

    SDL_ShowCursor(SDL_DISABLE);

    /*
     * Ask SDL for an OpenGL ES 2.0 context.
     */
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_ES
    );

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_MAJOR_VERSION,
        2
    );

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_MINOR_VERSION,
        0
    );

    SDL_GL_SetAttribute(
        SDL_GL_DOUBLEBUFFER,
        1
    );

    SDL_GL_SetAttribute(
        SDL_GL_DEPTH_SIZE,
        16
    );

    SDL_Window *window = SDL_CreateWindow(
        "SDL2 OpenGL ES",
        0,
        0,
        640,
        480,
        SDL_WINDOW_OPENGL
    );

    if (!window) {
        fprintf(stderr,
                "SDL_CreateWindow failed: %s\n",
                SDL_GetError());

        SDL_Quit();
        return 1;
    }

    SDL_GLContext context =
        SDL_GL_CreateContext(window);

    if (!context) {
        fprintf(stderr,
                "SDL_GL_CreateContext failed: %s\n",
                SDL_GetError());

        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("GL renderer: %s\n",
           glGetString(GL_RENDERER));

    printf("GL version: %s\n",
           glGetString(GL_VERSION));

    int running = 1;

    while (running) {

        SDL_Event event;

        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT)
                running = 0;

            if (event.type == SDL_KEYDOWN)
                running = 0;
        }

        glClearColor(
            0.1f,
            0.9f,
            0.2f,
            1.0f
        );

        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

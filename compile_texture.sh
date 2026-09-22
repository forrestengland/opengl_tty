gcc -g -O0 opengl_texture.c -o opengl_texture \
    $(sdl2-config --cflags --libs) \
    -lGLESv2 -lm -lSDL2_ttf

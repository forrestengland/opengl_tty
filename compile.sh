gcc test.c -o test \
    $(sdl2-config --cflags --libs) \
    -lGLESv2

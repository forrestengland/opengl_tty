gcc opengl_obj.c -o opengl_obj \
    $(sdl2-config --cflags --libs) \
    -lGLESv2

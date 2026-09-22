// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// sdl
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// opengl
#include <GLES2/gl2.h>

// stb image for loading bitmap textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// custom libraries
#include "f3_vec.h"
#include "f3_mat.h"
#include "f3_obj.h"
#include "player.h"

// texture map image info
int image_width, image_height, image_channels;
unsigned char* image_data = 0;

// requested screen size for desktop
#define SCREEN_W 640
#define SCREEN_H 480

// wireframe or solid display for player object
#define WIREFRAME 0

// player object file
#define OBJFILE "cube.obj"

// texture image
#define IMAGEFILE "cube_texture.bmp"

// pi
#define PI 3.1415926535

// show cursor or not
#define SHOW_CURSOR 0

// stuff gl needs access to
GLuint program;
GLuint vertexBuffer;
GLuint planeVertexBuffer;
GLuint texture;
GLint positionAttribute;
GLint matrixUniform;
GLint projectionUniform;
GLint viewUniform;
GLint textureUniform;

// 'hud' stuff
GLuint hudProgram;
GLuint hudVertexBuffer;
GLuint hudTexture;
GLint hudTextureUniform;
GLint hudPositionAttribute;
GLint hudTexCoordAttribute;
GLint hudScreenSizeUniform;

// Time
double getTime() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec +
           ts.tv_nsec / 1000000000.0;
}

// 3d vertex shader program
const char *vertexShaderSource =
  "attribute vec3 position;\n"
  "attribute vec3 normal;\n"
  "attribute vec2 texCoord;\n"
  "uniform mat4 modelMatrix;\n"
  "uniform mat4 projectionMatrix;\n"
  "uniform mat4 viewMatrix;\n"  
  "varying vec3 vertexNormal;\n"
  "varying vec2 vertexTexCoord;\n"
  "\n"
  "void main() {\n"
  "    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);\n"
  "    vertexNormal = normal;\n"
  "    vertexTexCoord = texCoord;\n"
  "}\n";

// 3d fragment shader program
const char *fragmentShaderSource =
  "precision mediump float;\n"
  "varying vec3 vertexNormal;\n"
  "varying vec2 vertexTexCoord;\n"
  "uniform sampler2D textureSampler;\n"
  "\n"
  "void main()\n"
  "{\n"
  "    vec3 lightDirection = normalize(vec3(1.0, 1.0, 1.0));\n"
  "\n"
  "    float brightness = max(\n"
  "        dot(normalize(vertexNormal), lightDirection),\n"
  "        0.0\n"
  "    );\n"
  "\n"
  "    float ambient = 0.2;\n"
  "\n"
  //  "    vec3 color = vec3(0.1, 0.8, 0.4);\n"
  "    vec4 texColor = texture2D(textureSampler, vertexTexCoord);\n"
  "\n"
  "    gl_FragColor = vec4(\n"
  "        texColor.rgb * (ambient + brightness),\n"
  "        texColor.a\n"
  "    );\n"
  "}\n";

// hud vertex shader program
const char *hudVertexShaderSource =
    "attribute vec2 position;\n"
    "attribute vec2 texCoord;\n"
    "uniform vec2 screenSize;\n"
    "varying vec2 vertexTexCoord;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec2 ndc = (position / screenSize) * 2.0 - 1.0;\n"
    "\n"
    "    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);\n"
    "    vertexTexCoord = texCoord;\n"
    "}\n";

// hud fragment shader program
const char *hudFragmentShaderSource =
    "precision mediump float;\n"
    "varying vec2 vertexTexCoord;\n"
    "uniform sampler2D textureSampler;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(textureSampler, vertexTexCoord);\n"
    "}\n";

// compile a shader program
GLuint compileShader(GLenum type, const char *source) {

  GLuint shader = glCreateShader(type);

  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    char log[512];

    glGetShaderInfoLog(shader, sizeof(log), NULL, log);

    fprintf(stderr, "Shader compilation failed:\n%s\n", log);

    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

// create a shader program
GLuint createProgram(void) {

  GLuint vertexShader =compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (!vertexShader || !fragmentShader)
        return 0;

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glBindAttribLocation(program, 0, "position");
    glBindAttribLocation(program, 1, "normal");
    glBindAttribLocation(program, 2, "texCoord");

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
      
        char log[512];

        glGetProgramInfoLog(program, sizeof(log), NULL, log);

        fprintf(stderr, "Program linking failed:\n%s\n", log);

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// create the hud 2d overlay shader programs
GLuint createHudProgram(void)
{

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, hudVertexShaderSource);
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, hudFragmentShaderSource);

    if (!vertexShader || !fragmentShader)
        return 0;

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glBindAttribLocation(program, 0, "position");
    glBindAttribLocation(program, 1, "texCoord");    

    glLinkProgram(program);

    GLint success;

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
      
        char log[512];

        glGetProgramInfoLog(program, sizeof(log), NULL, log);

        fprintf(stderr, "HUD program linking failed:\n%s\n", log);

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// draw ground plane
void draw_plane(Mat4 *model, Mat4 *view, Mat4 *projection)
{
    glUseProgram(program);

    // use the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUniform1i(textureUniform, 0);
    glUniformMatrix4fv(matrixUniform, 1, GL_FALSE, model->m);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, projection->m);
    glUniformMatrix4fv(viewUniform, 1, GL_FALSE, view->m);    
    glBindBuffer(GL_ARRAY_BUFFER, planeVertexBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

void draw_hud(int screenWidth, int screenHeight) {

  float rect[] = {
    10.0f, 10.0f,   0.0f, 0.0f,
    110.0f, 10.0f,  1.0f, 0.0f,
    110.0f, 60.0f,  1.0f, 1.0f,

    10.0f, 10.0f,   0.0f, 0.0f,
    110.0f, 60.0f,  1.0f, 1.0f,
    10.0f, 60.0f,   0.0f, 1.0f
  };

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  
  glUseProgram(hudProgram);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, hudTexture);
  
  glUniform1i(hudTextureUniform, 0);  

  // Upload rectangle vertices.
  glBindBuffer(GL_ARRAY_BUFFER, hudVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_DYNAMIC_DRAW);

  // Position attribute.
  glEnableVertexAttribArray(hudPositionAttribute);
  glVertexAttribPointer(hudPositionAttribute, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(hudTexCoordAttribute);
  glVertexAttribPointer(
			hudTexCoordAttribute,
			2,
			GL_FLOAT,
			GL_FALSE,
			4 * sizeof(float),
			(void *)(2 * sizeof(float))
			);

  // Tell the shader how large the screen is.
  glUniform2f(hudScreenSizeUniform, (float)screenWidth, (float)screenHeight);
  // Draw two triangles = rectangle.
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(hudPositionAttribute);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Draw OBJ model
void draw_model(Mat4 *model, Mat4 *view, Mat4 *projection, f3_obj* player_obj) {
  
  glUseProgram(program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(textureUniform,	0);
  glUniformMatrix4fv(matrixUniform, 1, GL_FALSE, model->m);
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, projection->m);
  glUniformMatrix4fv(viewUniform, 1, GL_FALSE, view->m);    
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  // Position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  // Normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
  // texcoords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

  // here we can draw a wireframe or solid model
  if (WIREFRAME) {
    for (int i = 0; i < player_obj->draw_vertex_count; i += 3) {
      glDrawArrays(GL_LINE_LOOP, i, 3);
    }
  } else {
    glDrawArrays(GL_TRIANGLES, 0, player_obj->draw_vertex_count);
  }

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

void updateHudTexture(int fps, TTF_Font* font) {

  // prepare text display
    SDL_Color white = {255, 255, 255, 255};

    char fpstext[255];
    snprintf(fpstext, 255, "FPS: %d", fps);
    
    SDL_Surface *textSurface =
      TTF_RenderText_Blended(font, fpstext, white);

    if (!textSurface) {
      fprintf(stderr, "Failed to render text: %s\n", TTF_GetError());
    }

    SDL_Surface *rgbaSurface =
    SDL_ConvertSurfaceFormat(
			     textSurface,
			     SDL_PIXELFORMAT_RGBA32,
			     0
			     );

    SDL_FreeSurface(textSurface);    

    if (!rgbaSurface) {
      fprintf(stderr, "Failed to convert text surface: %s\n",
	      SDL_GetError());
      return;
    }

    glBindTexture(GL_TEXTURE_2D, hudTexture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
		 GL_TEXTURE_2D,
		 0,
		 GL_RGBA,
		 rgbaSurface->w,
		 rgbaSurface->h,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_BYTE,
		 rgbaSurface->pixels
		 );

    SDL_FreeSurface(rgbaSurface);    
}

// main program entry
int main(int argc, char* argv[]) {

  player p;
  player_init(&p);

  f3_obj player_obj;
  f3_obj_init(&player_obj);
  player_obj.filename = OBJFILE;

  // init sdl2
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fprintf(stderr,
	    "SDL_Init failed: %s\n",
	    SDL_GetError());
    return 1;
  }
  if (TTF_Init() != 0) {
    fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    return 1;
  }
  TTF_Font *font = TTF_OpenFont("SuperMaples-2vR2w.ttf", 24);
  if (!font) {
    fprintf(stderr, "failed to load font '%s'\n", TTF_GetError());
    return 1;
  }

  printf("SDL video driver: %s\n",
       SDL_GetCurrentVideoDriver());

  // mouse cursor
  if (!SHOW_CURSOR)
    SDL_ShowCursor(SDL_DISABLE);

  // Ask SDL for an OpenGL ES 2.0 context.
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  SDL_Window *window = SDL_CreateWindow("SDL2 OpenGL ES", 0, 0,	SCREEN_W, SCREEN_H, SDL_WINDOW_OPENGL);

  if (!window) {
    
    fprintf(stderr, "SDL_CreateWindow failed: %s\n",SDL_GetError());

    SDL_Quit();
    return 1;
  }

  int numKeys;
  SDL_GetKeyboardState(&numKeys);
  printf("Number of keys: %d\n", numKeys);
  printf("Num joysticks: %d\n", SDL_NumJoysticks());

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context) {
    
    fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  printf("GL renderer: %s\n", glGetString(GL_RENDERER));
  printf("GL version: %s\n", glGetString(GL_VERSION));

  // enable depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearDepthf(1.0f);
  int depthBits;
  SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depthBits);
  printf("Depth buffer: %d bits\n", depthBits);

  // create the gpu program
  program = createProgram();

  if (!program) {
    fprintf(stderr, "Failed to create shader program\n");

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 1;
  }

  // handle to communicate with the shader program
  matrixUniform = glGetUniformLocation(program, "modelMatrix");
  projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  textureUniform = glGetUniformLocation(program, "textureSampler");
  viewUniform = glGetUniformLocation(program, "viewMatrix");

  hudProgram = createHudProgram();

  if (!hudProgram) {
    fprintf(stderr, "Failed to create HUD shader program\n");

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 1;
  }

  hudPositionAttribute =
    glGetAttribLocation(hudProgram, "position");

  hudTexCoordAttribute =
    glGetAttribLocation(hudProgram, "texCoord");  

  hudScreenSizeUniform = glGetUniformLocation(hudProgram, "screenSize");

  // hud buffer
  glGenBuffers(1, &hudVertexBuffer);

  // hud text texture
  glGenTextures(1, &hudTexture);

  glBindTexture(GL_TEXTURE_2D, hudTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  hudTextureUniform = glGetUniformLocation(hudProgram, "textureSampler");
  
  // running
  int running = 1;

  // load image for texture
  stbi_set_flip_vertically_on_load(1);
  image_data = stbi_load(IMAGEFILE, &image_width, &image_height,
			 &image_channels, 0);
  if (image_data == NULL) {
    printf("error loading texture image data '%s'\n", IMAGEFILE);
    return 1;
  }

  printf("loaded texture: %d x %d, %d channels\n", image_width, image_height,
	 image_channels);

  // load texture
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0,
	       GL_RGB, GL_UNSIGNED_BYTE, image_data);

  if (!f3_obj_load(&player_obj)) return 1;

  float *model_vertices = f3_obj_model_vertices(&player_obj);
  if (!model_vertices) {
    printf("getting model vertices failed\n");
    return 1;
  }

  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, player_obj.draw_vertex_count * 8 * sizeof(float),
	       model_vertices, GL_STATIC_DRAW);

  free(model_vertices);

  // create ground plane
  float plane_vertices[] = {
    /* position         normal        texcoord */

    -5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
     5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
     5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,

    -5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
     5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
    -5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
  };

  glGenBuffers(1, &planeVertexBuffer);

  glBindBuffer(GL_ARRAY_BUFFER, planeVertexBuffer);

  glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);

  int width;
  int height;

  SDL_GL_GetDrawableSize(window, &width, &height);

  glViewport(0, 0, width, height);

  float aspect = (float)width / (float)height;

  Mat4 projection = mat4_perspective(60.0f * PI / 180.0f, aspect, 0.1f, 100.0f);

  // Timing
  double previousTime = getTime();
  double angleX = 0.0;
  double angleY = 0.0;  
  int frameCount = 0;
  double fpsTimer = 0.0;
  int fps = 0;

  int jumpWasDown = 0;

  // Main loop
  while (running) {

    double currentTime = getTime();
    double deltaTime = currentTime - previousTime;
    previousTime = currentTime;

    // FPS
    frameCount++;
    fpsTimer += deltaTime;
    if (fpsTimer >= 1.0) {
      printf("FPS: %d\n", frameCount);
      fps = frameCount;
      frameCount = 0;
      fpsTimer = 0.0;
      updateHudTexture(fps, font);
    }

    // Events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
	running = 0;
      if (event.type == SDL_KEYDOWN) {
        printf("Key pressed: %s\n",
               SDL_GetKeyName(event.key.keysym.sym));
	if (event.key.keysym.sym == SDLK_ESCAPE) {
	  running = 0;
	}
      }

      if (event.type == SDL_KEYUP) {
        printf("Key released: %s\n",
               SDL_GetKeyName(event.key.keysym.sym));
      }      
    }

    // keyboard -> movement
    const Uint8* keyboard = SDL_GetKeyboardState(NULL);
    if (keyboard[SDL_SCANCODE_W]) {
      player_move_forward(&p, deltaTime);
    }
    if (keyboard[SDL_SCANCODE_S]) {
      player_move_backward(&p, deltaTime);
    }
    if (keyboard[SDL_SCANCODE_A]) {
      player_move_left(&p, deltaTime);
    }
    if (keyboard[SDL_SCANCODE_D]) {
      player_move_right(&p, deltaTime);
    }

    // jump
    int jumpDown = keyboard[SDL_SCANCODE_SPACE];
    if (jumpDown && !jumpWasDown) {
      if (player_can_jump(&p)) {
	player_jump(&p);
      }
    }
    jumpWasDown = jumpDown;

    // update player y position based on gravity
    player_do_gravity(&p, deltaTime);
    
    // update camera based on player
    Vec3 cameraPosition = {p.pos_x, p.pos_y + 1.0f, p.pos_z + 2.0f};
    Vec3 cameraTarget = {p.pos_x, p.pos_y, p.pos_z};
    Vec3 cameraUp = {0.0f, 1.0f, 0.0f};
    Mat4 view = mat4_look_at(cameraPosition, cameraTarget, cameraUp);

    // update Rotation
    angleX += p.rot_x * deltaTime;
    if (angleX >= 360.0) {
      angleX -= 360.0;
    }
    angleY += p.rot_y * deltaTime;
    if (angleY >= 360.0) {
      angleY -= 360.0;
    }

    // calculate matrices
    Mat4 rotationX = mat4_rotation_x((float)(angleX * PI / 180.0));
    Mat4 rotationY = mat4_rotation_y((float)(angleY * PI / 180.0));
    Mat4 rotation = mat4_multiply(rotationY, rotationX);
    
    Mat4 translation = mat4_translation(p.pos_x, p.pos_y, p.pos_z);    
    Mat4 model = mat4_multiply(translation, rotation);

    Mat4 planeTranslation = mat4_translation(0.0f, -0.25f, -3.0f);

    // Clear frame
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // draw ground
    draw_plane(&planeTranslation, &view, &projection);

    // Draw OBJ
    draw_model(&model, &view, &projection, &player_obj);

    // draw hud
    draw_hud(width, height);

    // Display frame
    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  f3_obj_cleanup(&player_obj);

  glDeleteTextures(1, &texture);
  glDeleteBuffers(1, &vertexBuffer);
  glDeleteBuffers(1, &planeVertexBuffer);
  glDeleteProgram(program);
 
  glDeleteTextures(1, &hudTexture);
  glDeleteBuffers(1, &hudVertexBuffer);
  glDeleteProgram(hudProgram);

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

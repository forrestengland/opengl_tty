#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <GLES2/gl2.h>

#include <time.h>
#include <math.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int image_width, image_height, image_channels;
unsigned char* image_data = 0;

#define WIREFRAME 0
#define CAMERA_DISTANCE -1.0
#define OBJFILE "cube.obj"
#define IMAGEFILE "cube_texture.bmp"

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

#define ROTATION_SPEED 30.0

#define PI 3.1415926535

GLuint program;
GLuint vertexBuffer;
GLuint texture;

GLint positionAttribute;

GLint matrixUniform;
GLint projectionUniform;
GLint textureUniform;

// number of vertices to send to glDrawArrays()
int draw_vertex_count = 0;

// Basic 3D types
typedef struct {
    float x;
    float y;
    float z;
} Vec3;

// 2d vector for textures
typedef struct {
  float u;
  float v;
} Vec2;

// 4x4 column major matrix
typedef struct {
    float m[16];
} Mat4;

typedef struct {
  int v[3];   /* vertex indices */
  int t[3]; /* texture coords */
  int n[3];   /* normal indices */
} Face;

// OBJ data
Vec3 *vertices = NULL;
int vertex_count = 0;
int vertex_capacity = 0;

Vec3 *normals = NULL;
int normal_count = 0;
int normal_capacity = 0;

Vec2 *texcoords = NULL;
int texcoord_count = 0;
int texcoord_capacity = 0;

Face *faces = NULL;
int face_count = 0;
int face_capacity = 0;

// identity matrix
Mat4 mat4_identity(void)
{
    Mat4 result = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};

    return result;
}

// x rotation matrix 
Mat4 mat4_rotation_x(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    Mat4 result = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,    c,    s, 0.0f,
        0.0f,   -s,    c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};

    return result;
}

Mat4 mat4_rotation_y(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    Mat4 result = {{
         c, 0.0f, -s, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f,
         s, 0.0f,  c, 0.0f,
       0.0f, 0.0f, 0.0f, 1.0f
    }};

    return result;
}

// translation matrix
Mat4 mat4_translation(float x, float y, float z)
{
    Mat4 result = mat4_identity();

    result.m[12] = x;
    result.m[13] = y;
    result.m[14] = z;

    return result;
}

// matrix multiplication
Mat4 mat4_multiply(Mat4 a, Mat4 b)
{
    Mat4 result;

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {

            result.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }

    return result;
}

// perspective matrix
Mat4 mat4_perspective(
    float fov,
    float aspect,
    float near,
    float far
) {
    float f = 1.0f / tanf(fov / 2.0f);

    Mat4 result = {{
        f / aspect, 0.0f, 0.0f, 0.0f,

        0.0f, f, 0.0f, 0.0f,

        0.0f, 0.0f,
        (far + near) / (near - far),
        -1.0f,

        0.0f, 0.0f,
        (2.0f * far * near) / (near - far),
        0.0f
    }};

    return result;
}

// Time
double getTime() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec +
           ts.tv_nsec / 1000000000.0;
}


// Add OBJ vertex
void add_vertex(Vec3 v) {
    if (vertex_count >= vertex_capacity) {

        vertex_capacity =
            vertex_capacity == 0 ? 64 : vertex_capacity * 2;

        vertices = realloc(
            vertices,
            vertex_capacity * sizeof(Vec3)
        );

        if (!vertices) {
            fprintf(stderr, "Failed to allocate vertices\n");
            exit(1);
        }
    }

    vertices[vertex_count++] = v;
}


// Add OBJ normal
void add_normal(Vec3 n) {
    if (normal_count >= normal_capacity) {

        normal_capacity =
            normal_capacity == 0 ? 64 : normal_capacity * 2;

        normals = realloc(
            normals,
            normal_capacity * sizeof(Vec3)
        );

        if (!normals) {
            fprintf(stderr, "Failed to allocate normals\n");
            exit(1);
        }
    }

    normals[normal_count++] = n;
}

// add texture coordinate
void add_texcoord(Vec2 t)
{
    if (texcoord_count >= texcoord_capacity) {

        texcoord_capacity =
            texcoord_capacity == 0 ? 64 : texcoord_capacity * 2;

        texcoords = realloc(
            texcoords,
            texcoord_capacity * sizeof(Vec2)
        );

        if (!texcoords) {
            fprintf(stderr, "Failed to allocate texcoords\n");
            exit(1);
        }
    }

    texcoords[texcoord_count++] = t;
}

// Add triangle
void add_face(Face f) {
    if (face_count >= face_capacity) {

        face_capacity =
            face_capacity == 0 ? 64 : face_capacity * 2;

        faces = realloc(
            faces,
            face_capacity * sizeof(Face)
        );

        if (!faces) {
            fprintf(stderr, "Failed to allocate faces\n");
            exit(1);
        }
    }

    faces[face_count++] = f;
}


// Load OBJ
int load_obj(const char *filename) {
    FILE *file = fopen(filename, "r");

    if (!file) {
        fprintf(stderr,
                "Failed to open OBJ file: %s\n",
                filename);
        return 0;
    }

    /*
     * Wings3D can produce fairly long face lines,
     * so don't use a tiny 256 byte buffer.
     */
    char line[4096];

    while (fgets(line, sizeof(line), file)) {

        /*
         * Vertex position:
         *
         * v x y z
         */
      if (line[0] == 'v' && line[1] == ' ') {

	Vec3 v;

	if (sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z) == 3) {
                add_vertex(v);
	}

	// texture coordinate
      } else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {

	Vec2 t;

	if (sscanf(line, "vt %f %f", &t.u, &t.v) == 2) {
	  add_texcoord(t);
	}

	// Vertex normal:
      } else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {

	Vec3 n;

	if (sscanf(line, "vn %f %f %f", &n.x, &n.y, &n.z) == 3) {
	  add_normal(n);
	}
      }

        /*
         * Face:
         *
         * f 1//1 5//5 6//6
         *
         * or:
         *
         * f 1//1 5//5 6//6 2//2
         */
	//--------------------------
	else if (line[0] == 'f' &&
		 line[1] == ' ') {

	  int v[64];
	  int n[64];
	  int t[64];

	  int vertex_count = 0;

	  char *token = strtok(line + 2, " \t\r\n");

	  while (token && vertex_count < 64) {

	    /*
	     * Wings3D format:
	     *
	     *     vertex//normal
	     *
	     * Example:
	     *
	     *     12//12
	     */

	    int vertex_index;
	    int normal_index;
	    int texture_index;

	    if (sscanf(token, "%d/%d/%d",
		       &vertex_index,
		       &texture_index,
		       &normal_index) == 3) {

	      v[vertex_count] = vertex_index - 1;
	      t[vertex_count] = texture_index - 1;
	      n[vertex_count] = normal_index - 1;

	      vertex_count++;
	    }
	    else if (sscanf(token, "%d//%d",
			    &vertex_index,
			    &normal_index) == 2) {

	      v[vertex_count] = vertex_index - 1;
	      n[vertex_count] = normal_index - 1;

	      vertex_count++;
	    }
	    token = strtok(NULL, " \t\r\n");
	  }


	  /*
	   * Need at least three vertices to
	   * make a polygon.
	   */
	  if (vertex_count >= 3) {

	    /*
	     * Triangle fan.
	     *
	     * Polygon:
	     *
	     *     0 --- 1
	     *    /       \
	     *   5         2
	     *    \       /
	     *     4 --- 3
	     *
	     * becomes:
	     *
	     *     0,1,2
	     *     0,2,3
	     *     0,3,4
	     *     0,4,5
	     */

	    for (int i = 1; i < vertex_count - 1; i++) {

	      Face f;

	      f.v[0] = v[0];
	      f.v[1] = v[i];
	      f.v[2] = v[i + 1];

	      f.n[0] = n[0];
	      f.n[1] = n[i];
	      f.n[2] = n[i + 1];

	      f.t[0] = t[0];
	      f.t[1] = t[i];
	      f.t[2] = t[i + 1];

	      add_face(f);
	    }
	  }
	}
    }

    fclose(file);

    printf(
        "Loaded OBJ: %d vertices, %d normals, %d triangles\n",
        vertex_count,
        normal_count,
        face_count
    );

    return 1;
}

const char *vertexShaderSource =
  "attribute vec3 position;\n"
  "attribute vec3 normal;\n"
  "attribute vec2 texCoord;\n"
  "uniform mat4 modelMatrix;\n"
  "uniform mat4 projectionMatrix;\n"
  "varying vec3 vertexNormal;\n"
  "varying vec2 vertexTexCoord;\n"
  "\n"
  "void main() {\n"
  "    gl_Position = projectionMatrix * modelMatrix * vec4(position, 1.0);\n"
  "    vertexNormal = normal;\n"
  "    vertexTexCoord = texCoord;\n"
  "}\n";

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

GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[512];

        glGetShaderInfoLog(
            shader,
            sizeof(log),
            NULL,
            log
        );

        fprintf(stderr,
                "Shader compilation failed:\n%s\n",
                log);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint createProgram(void)
{
    GLuint vertexShader =
        compileShader(
            GL_VERTEX_SHADER,
            vertexShaderSource
        );

    GLuint fragmentShader =
        compileShader(
            GL_FRAGMENT_SHADER,
            fragmentShaderSource
        );

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
    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &success
    );

    if (!success) {
        char log[512];

        glGetProgramInfoLog(
            program,
            sizeof(log),
            NULL,
            log
        );

        fprintf(stderr,
                "Program linking failed:\n%s\n",
                log);

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Draw OBJ model
void draw_model(Mat4 *model, Mat4 *projection) {
  
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUniform1i(textureUniform,	0);

    glUniformMatrix4fv(matrixUniform, 1, GL_FALSE, model->m);

    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, projection->m);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    // Position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			  8 * sizeof(float), (void *)0);

    // Normal
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			  8 * sizeof(float), (void *)(3 * sizeof(float)));

    // texcoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
			  8 * sizeof(float), (void *)(6 * sizeof(float)));

    if (WIREFRAME) {
      for (int i = 0; i < draw_vertex_count; i += 3) {
	glDrawArrays(GL_LINE_LOOP, i, 3);
      }
    } else {
      glDrawArrays(
		   GL_TRIANGLES,
		   0,
		   draw_vertex_count
		   );
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

// Main
int main(int argc, char* argv[]) {

  // init sdl2
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr,
	    "SDL_Init failed: %s\n",
	    SDL_GetError());
    return 1;
  }

  // don't show the mouse cursor
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

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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

  SDL_GLContext context = SDL_GL_CreateContext(window);
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
  //  angleUniform = glGetUniformLocation(program, "angle");
  matrixUniform = glGetUniformLocation(program, "modelMatrix");
  projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  textureUniform = glGetUniformLocation(program, "textureSampler");
  
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
  glTexParameteri(
		  GL_TEXTURE_2D,
		  GL_TEXTURE_MIN_FILTER,
		  GL_LINEAR
		  );

  glTexParameteri(
		  GL_TEXTURE_2D,
		  GL_TEXTURE_MAG_FILTER,
		  GL_LINEAR
		  );

  glTexParameteri(
		  GL_TEXTURE_2D,
		  GL_TEXTURE_WRAP_S,
		  GL_REPEAT
		  );

  glTexParameteri(
		  GL_TEXTURE_2D,
		  GL_TEXTURE_WRAP_T,
		  GL_REPEAT
		  );  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0,
	       GL_RGB, GL_UNSIGNED_BYTE, image_data);

  // Load OBJ
  char* filename = OBJFILE; // default
  /*  if (argc > 1) filename = argv[1]; // from command line arg */
  if (!load_obj(filename)) {
    return 1;
  }

  printf("Texture coordinates: %d\n", texcoord_count);

  for (int i = 0; i < texcoord_count; i++) {
    printf(
	   "%d: u=%f v=%f\n",
	   i,
	   texcoords[i].u,
	   texcoords[i].v
	   );
  }

  // send obj faces to gpu
  draw_vertex_count = face_count * 3;

  float *model_vertices = malloc(draw_vertex_count * 8 * sizeof(float));

  if (!model_vertices) {
    fprintf(stderr, "Failed to allocate model vertices\n");
    return 1;
  }

  float scale = 0.2f;

  int index = 0;

  for (int i = 0; i < face_count; i++) {

    Face *face = &faces[i];

    for (int j = 0; j < 3; j++) {

      Vec3 *v = &vertices[face->v[j]];
      Vec3 *n = &normals[face->n[j]];
      Vec2 *t = &texcoords[face->t[j]];

      // position
      model_vertices[index++] = v->x * scale;
      model_vertices[index++] = v->y * scale;
      model_vertices[index++] = v->z * scale;

      // normal
      model_vertices[index++] = n->x;
      model_vertices[index++] = n->y;
      model_vertices[index++] = n->z;

      // texture coordinates
      model_vertices[index++] = t->u;
      model_vertices[index++] = t->v;      
    }
  }  

  glGenBuffers(1, &vertexBuffer);

  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

  glBufferData(GL_ARRAY_BUFFER, draw_vertex_count * 8 * sizeof(float),
	       model_vertices, GL_STATIC_DRAW);

  free(model_vertices);

  Mat4 projection = mat4_perspective(60.0f * PI / 180.0f,
				     (float)WINDOW_WIDTH / WINDOW_HEIGHT,
				     0.1f,
				     100.0f);

  // Timing
  double previousTime = getTime();
  double angle = 0.0;
  double rotationSpeed = ROTATION_SPEED;
  int frameCount = 0;
  double fpsTimer = 0.0;

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
      frameCount = 0;
      fpsTimer = 0.0;
    }

    // Events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
	running = 0;
      if (event.type == SDL_KEYDOWN)
	running = 0;
    }

    // update Rotation
    angle += rotationSpeed * deltaTime;
    if (angle >= 360.0) {
      angle -= 360.0;
    }

    // calculate matrices
    Mat4 rotationX = mat4_rotation_x((float)(angle * PI / 180.0));
    Mat4 rotationY = mat4_rotation_y((float)(angle * PI / 180.0));    
    Mat4 rotation = mat4_multiply(rotationY, rotationX);
    Mat4 translation = mat4_translation(0.0f, 0.0f, CAMERA_DISTANCE);
    Mat4 model = mat4_multiply(translation, rotation);
    
    // Clear frame
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Model color.

    // Draw OBJ
    draw_model(&model, &projection);

    // Display frame
    SDL_GL_SwapWindow(window);
  }

  // Cleanup

  free(vertices);
  free(normals);
  free(faces);

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

    return 0;
}

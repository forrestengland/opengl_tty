#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <GLES2/gl2.h>

#include <time.h>
#include <math.h>
#include <string.h>

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

#define ROTATION_SPEED 10.0

// Basic 3D types
typedef struct {
    float x;
    float y;
    float z;
} Vec3;


typedef struct {
    int v[3];   /* vertex indices */
    int n[3];   /* normal indices */
} Face;


// OBJ data
Vec3 *vertices = NULL;
int vertex_count = 0;
int vertex_capacity = 0;

Vec3 *normals = NULL;
int normal_count = 0;
int normal_capacity = 0;

Face *faces = NULL;
int face_count = 0;
int face_capacity = 0;


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
        if (line[0] == 'v' &&
            line[1] == ' ') {

            Vec3 v;

            if (sscanf(
                    line,
                    "v %f %f %f",
                    &v.x,
                    &v.y,
                    &v.z
                ) == 3) {

                add_vertex(v);
            }
        }

        /*
         * Vertex normal:
         *
         * vn x y z
         */
        else if (line[0] == 'v' &&
                 line[1] == 'n' &&
                 line[2] == ' ') {

            Vec3 n;

            if (sscanf(
                    line,
                    "vn %f %f %f",
                    &n.x,
                    &n.y,
                    &n.z
                ) == 3) {

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

	    if (sscanf(
		       token,
		       "%d//%d",
		       &vertex_index,
		       &normal_index
		       ) == 2) {

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

GLuint program;
GLuint vertexBuffer;
GLint positionAttribute;

const char *vertexShaderSource =
    "attribute vec3 position;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(position, 1.0);\n"
    "}\n";

const char *fragmentShaderSource =
    "precision mediump float;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = vec4(0.1, 0.8, 0.4, 1.0);\n"
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

    glBindAttribLocation(
        program,
        0,
        "position"
    );

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
void draw_model() {
  
  //    glBegin(GL_TRIANGLES);

    for (int i = 0; i < face_count; i++) {

        Face f = faces[i];

        for (int j = 0; j < 3; j++) {

            Vec3 n = normals[f.n[j]];
            Vec3 v = vertices[f.v[j]];

            /*
             * Tell OpenGL which normal belongs
             * to this vertex.
             */
	    //        glNormal3f(
	    //                n.x,
	    //                n.y,
	    //                n.z
	    //            );

            /*
             * Then specify the vertex.
             */
	    //            glVertex3f(
	    //                v.x,
	    //                v.y,
	    //                v.z
	    //            );
        }
    }

    //    glEnd();

    glUseProgram(program);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        vertexBuffer
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,              // attribute number
        2,              // x,y
        GL_FLOAT,
        GL_FALSE,
        0,
        0
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        3
    );

    glDisableVertexAttribArray(0);
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

  // create the gpu program
  program = createProgram();

  if (!program) {
    fprintf(stderr, "Failed to create shader program\n");

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 1;
  }

  // test triangle
  float triangle[] = {
     0.0f,  0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f
  };

  glGenBuffers(1, &vertexBuffer);

  glBindBuffer(
	       GL_ARRAY_BUFFER,
	       vertexBuffer
	       );

  glBufferData(
	       GL_ARRAY_BUFFER,
	       sizeof(triangle),
	       triangle,
	       GL_STATIC_DRAW
	       );

  
  int running = 1;

  // Load OBJ
  char* filename = "shape.obj"; // default
  if (argc > 1) filename = argv[1]; // from command line arg
  if (!load_obj(filename)) {
    return 1;
  }

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
    
    // Rotation
    angle += rotationSpeed * deltaTime;
    if (angle >= 360.0) {
      angle -= 360.0;
    }

    // Clear frame
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // ModelView transformation

    // Move model away from camera.

    // Model color.

    // Draw OBJ
    draw_model();


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

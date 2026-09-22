#ifndef H_F3_OBJ
#define H_F3_OBJ

#include <stdio.h>
#include <stdlib.h>

#include "f3_vec.h"
#include "f3_mat.h"

// face definition
typedef struct {
  int v[3];   /* vertex indices */
  int t[3]; /* texture coords */
  int n[3];   /* normal indices */
} Face;

typedef struct f3_obj_s {

  char* filename;

  int draw_vertex_count;
  
  Vec3 *vertices;
  int vertex_count;
  int vertex_capacity;

  // normals
  Vec3 *normals;
  int normal_count;
  int normal_capacity;

  // texture coordinates
  Vec2 *texcoords;
  int texcoord_count;
  int texcoord_capacity;

  // faces
  Face *faces;
  int face_count;
  int face_capacity;
  
} f3_obj;

void f3_obj_init(f3_obj* o) {

  o->filename = "";
  o->draw_vertex_count = 0;

  o->vertices = NULL;
  o->vertex_count = 0;
  o->vertex_capacity = 0;

  // normals
  o->normals = NULL;
  o->normal_count = 0;
  o->normal_capacity = 0;

  // texture coordinates
  o->texcoords = NULL;
  o->texcoord_count = 0;
  o->texcoord_capacity = 0;

  // faces
  o->faces = NULL;
  o->face_count = 0;
  o->face_capacity = 0;
}

// Add OBJ vertex
void f3_obj_add_vertex(f3_obj* o, Vec3 v) {
  
    if (o->vertex_count >= o->vertex_capacity) {

        o->vertex_capacity = o->vertex_capacity == 0 ? 64 : o->vertex_capacity * 2;

        o->vertices = realloc(o->vertices, o->vertex_capacity * sizeof(Vec3));

        if (!o->vertices) {
            fprintf(stderr, "Failed to allocate vertices\n");
            exit(1);
        }
    }

    o->vertices[o->vertex_count++] = v;
}

// add texture coordinate
void f3_obj_add_texcoord(f3_obj* o, Vec2 t)
{
    if (o->texcoord_count >= o->texcoord_capacity) {

        o->texcoord_capacity = o->texcoord_capacity == 0 ? 64 : o->texcoord_capacity * 2;

        o->texcoords = realloc(o->texcoords, o->texcoord_capacity * sizeof(Vec2));

        if (!o->texcoords) {
            fprintf(stderr, "Failed to allocate texcoords\n");
            exit(1);
        }
    }

    o->texcoords[o->texcoord_count++] = t;
}

// Add OBJ normal
void f3_obj_add_normal(f3_obj* o, Vec3 n) {

  if (o->normal_count >= o->normal_capacity) {

    o->normal_capacity = o->normal_capacity == 0 ? 64 : o->normal_capacity * 2;

    o->normals = realloc(o->normals, o->normal_capacity * sizeof(Vec3));

    if (!o->normals) {
      fprintf(stderr, "Failed to allocate normals\n");
      exit(1);
    }
  }

  o->normals[o->normal_count++] = n;
}

// Add triangle
void f3_obj_add_face(f3_obj* o, Face f) {
    if (o->face_count >= o->face_capacity) {

        o->face_capacity = o->face_capacity == 0 ? 64 : o->face_capacity * 2;

        o->faces = realloc(o->faces, o->face_capacity * sizeof(Face));

        if (!o->faces) {
            fprintf(stderr, "Failed to allocate faces\n");
            exit(1);
        }
    }

    o->faces[o->face_count++] = f;
}

// Load OBJ file
int f3_obj_load(f3_obj* o) {

  FILE *file = fopen(o->filename, "r");

    if (!file) {
        fprintf(stderr, "Failed to open OBJ file: %s\n", o->filename);
        return 0;
    }

     /* Wings3D can produce fairly long face lines,
	so don't use a tiny 256 byte buffer. */
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
	  f3_obj_add_vertex(o, v);
	}

	// texture coordinate
      } else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {

	Vec2 t;

	if (sscanf(line, "vt %f %f", &t.u, &t.v) == 2) {
	  f3_obj_add_texcoord(o, t);
	}

	// Vertex normal:
      } else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {

	Vec3 n;

	if (sscanf(line, "vn %f %f %f", &n.x, &n.y, &n.z) == 3) {
	  f3_obj_add_normal(o, n);
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

	      f3_obj_add_face(o, f);
	    }
	  }
	}
    }

    fclose(file);

    printf(
        "Loaded OBJ: %d vertices, %d normals, %d triangles\n",
        o->vertex_count,
        o->normal_count,
        o->face_count
    );

    return 1;
}

float* f3_obj_model_vertices(f3_obj* o) {

  // send obj faces to gpu
  o->draw_vertex_count = o->face_count * 3;
  float *model_vertices = malloc(o->draw_vertex_count * 8 * sizeof(float));
  if (!model_vertices) {
    fprintf(stderr, "Failed to allocate model vertices\n");
    return NULL;
  }

  float scale = 0.2f;
  int index = 0;
  for (int i = 0; i < o->face_count; i++) {
    Face *face = &o->faces[i];
    for (int j = 0; j < 3; j++) {

      Vec3 *v = &o->vertices[face->v[j]];
      Vec3 *n = &o->normals[face->n[j]];
      Vec2 *t = &o->texcoords[face->t[j]];

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

  return model_vertices;
}

void f3_obj_cleanup(f3_obj* o) {
  free(o->vertices);
  free(o->normals);
  free(o->faces);
}

#endif

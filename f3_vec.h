#ifndef H_F3_VEC
#define H_F3_VEC

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

// vector operations
Vec3 vec3_subtract(Vec3 a, Vec3 b) {
  Vec3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
  return result;
}

Vec3 vec3_normalize(Vec3 v) {
  float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  Vec3 result = {v.x / length, v.y / length, v.z / length};
  return result;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
  Vec3 result = {
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  };

  return result;
}

#endif


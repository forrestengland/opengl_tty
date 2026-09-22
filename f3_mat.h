#ifndef H_F3_MAT
#define H_F3_MAT

// 4x4 column major matrix
typedef struct {
    float m[16];
} Mat4;

// matrix functions
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

// y rotation matrix
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

Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {

  Vec3 forward = vec3_normalize(vec3_subtract(target, eye));

  Vec3 right = vec3_normalize(vec3_cross(forward, up));

  Vec3 cameraUp = vec3_cross(right, forward);

  Mat4 result = {{
      right.x,       cameraUp.x,      -forward.x,       0.0f,
      right.y,       cameraUp.y,      -forward.y,       0.0f,
      right.z,       cameraUp.z,      -forward.z,       0.0f,

      -(
	right.x * eye.x +
	right.y * eye.y +
	right.z * eye.z
	),

      -(
	cameraUp.x * eye.x +
	cameraUp.y * eye.y +
	cameraUp.z * eye.z
	),

      forward.x * eye.x +
      forward.y * eye.y +
      forward.z * eye.z,

      1.0f
    }};

  return result;
}

#endif

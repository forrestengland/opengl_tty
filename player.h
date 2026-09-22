#ifndef H_PLAYER
#define H_PLAYER

// speed player moves at
#define PLAYER_SPEED 2.0
// rotation speed of player object
#define ROTATION_SPEED_X 5.0
#define ROTATION_SPEED_Y 15.0
#define JUMP_VELOCITY 5.0
#define PLAYER_HALF_HEIGHT 0.2;
// constants for gravity and jumping
#define GRAVITY -9.8
#define GROUND_Y -0.25

typedef struct player_s {
  float speed;
  float rot_x;
  float rot_y;
  float vel_y;
  float jump_vel;
  float half_height;
  float gravity;
  float ground_y;
  float pos_x;
  float pos_y;
  float pos_z;
} player;

void player_init(player* p) {
  p->speed = PLAYER_SPEED;
  p->rot_x = ROTATION_SPEED_X;
  p->rot_y = ROTATION_SPEED_Y;
  p->vel_y = 0.0;
  p->jump_vel = JUMP_VELOCITY;
  p->half_height = PLAYER_HALF_HEIGHT;
  p->gravity = GRAVITY;
  p->ground_y = GROUND_Y;
  p->pos_x = 0.0;
  p->pos_y = p->ground_y + p->half_height;
  p->pos_z = 0.0;
}

void player_move_forward(player* p, float deltaTime) {
  p->pos_z -= p->speed * deltaTime;
}

void player_move_backward(player* p, float deltaTime) {
  p->pos_z += p->speed * deltaTime;
}

void player_move_left(player* p, float deltaTime) {
  p->pos_x -= p->speed * deltaTime;
}

void player_move_right(player* p, float deltaTime) {
  p->pos_x += p->speed * deltaTime;
}

int player_can_jump(player* p) {

  float groundPlayerY = p->ground_y + p->half_height;
  return p->pos_y <= groundPlayerY + 0.001f;
}

void player_jump(player* p) {
  p->vel_y = p->jump_vel;
}

void player_do_gravity(player* p, float deltaTime) {

  p->vel_y += p->gravity * deltaTime;
  p->pos_y += p->vel_y * deltaTime;

  float groundPlayerY = p->ground_y + p->half_height;
  if (p->pos_y < groundPlayerY) {
    p->pos_y = groundPlayerY;
    p->vel_y = 0.0f;
  }
}

#endif

#ifndef COMMON_H
#define COMMON_H

#define GLSL_VERSION 330


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define SAMPLE_SIZE                0.5
#define MEAN_SIZE                  SAMPLE_SIZE * 2
#define SAMPLE_COLOR               RED
#define N                          2 * 1000
#define K                          12
#define K_MAX                      256
#define CAMERA_SPEED               SAMPLE_SIZE *N
#define COLORS_COUNT               (sizeof(colors) / sizeof(colors[0]))
#define CAMERA_TRANSITION_DURATION 1.0

// Colors array for clusters
static Color colors[] = {RED,       GREEN,      BLUE, YELLOW, ORANGE,   VIOLET,    BROWN,    LIGHTGRAY, PINK,   GOLD,    MAGENTA,
                         DARKGREEN, DARKPURPLE, LIME, BROWN,  DARKBLUE, DARKBROWN, DARKGRAY, MAROON,    PURPLE, RAYWHITE};

typedef struct
{
  Vector3 *items;
  size_t   count;
  size_t   capacity;
} Samples3D;

extern Samples3D   set;
extern Samples3D   cluster[K_MAX];
extern Vector3     means[K_MAX];
extern Vector3     old_means[K_MAX];
extern Vector3     target_means[K_MAX];
extern Color       cluster_colors[K_MAX];
extern Color       old_colors[K_MAX];
extern Color       target_colors[K_MAX];
extern float       animation_time;
extern const float ANIMATION_DURATION;
extern bool        centroid_selected;
extern int         selected_centroid_index;
extern Vector3     camera_start_pos;
extern Vector3     camera_end_pos;
extern Vector3     camera_start_target;
extern Vector3     camera_end_target;
extern bool        camera_transition;
extern float       camera_transition_time;
extern float       camera_transition_duration;
extern size_t      current_k;

#endif
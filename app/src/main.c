#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include "main.h" // Ensure main.h contains necessary declarations

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

static Samples3D   set                        = {0};
static Samples3D   cluster[K_MAX]             = {0};
static Vector3     means[K_MAX]               = {0};
static Vector3     old_means[K_MAX]           = {0};
static Vector3     target_means[K_MAX]        = {0};
static Color       cluster_colors[K_MAX]      = {0};
static Color       old_colors[K_MAX]          = {0};
static Color       target_colors[K_MAX]       = {0};
static float       animation_time             = 0.0f;
static const float ANIMATION_DURATION         = 0.5f;
static bool        centroid_selected          = false;
static int         selected_centroid_index    = -1;
float              lamda                      = 5;
static Vector3     camera_start_pos           = {0};
static Vector3     camera_end_pos             = {0};
static Vector3     camera_start_target        = {0};
static Vector3     camera_end_target          = {0};
static bool        camera_transition          = false;
static float       camera_transition_time     = 0.0f;
static float       camera_transition_duration = 2.0f;

static void generate_cluster(Vector3 center, float radius, size_t count, Samples3D *samples)
{
  for(size_t i = 0; i < count; i++)
    {
      if(samples->capacity == 0)
        {
          samples->capacity = 1;
          samples->items    = malloc(samples->capacity * sizeof(Vector3));
        }
      if(samples->count == samples->capacity)
        {
          samples->capacity *= 2;
          samples->items = realloc(samples->items, samples->capacity * sizeof(Vector3));
        }
      float theta                    = GetRandomValue(0, 360) * DEG2RAD;
      float phi                      = GetRandomValue(0, 360) * DEG2RAD;
      float r                        = GetRandomValue(0, radius);
      float x                        = center.x + (r * sinf(theta) * cosf(phi));
      float y                        = center.y + (r * sinf(theta) * sinf(phi));
      float z                        = center.z + (r * cosf(theta));
      samples->items[samples->count] = (Vector3){x, y, z};
      samples->count++;
    }
}

static void randomize_means(size_t k, float bound)
{
  for(size_t i = 0; i < k; ++i)
    {
      means[i].x        = Lerp(-bound, bound, (float)GetRandomValue(0, 100) / 100);
      means[i].y        = Lerp(-bound, bound, (float)GetRandomValue(0, 100) / 100);
      means[i].z        = Lerp(-bound, bound, (float)GetRandomValue(0, 100) / 100);
      target_means[i]   = means[i];
      old_means[i]      = means[i];
      cluster_colors[i] = colors[i % COLORS_COUNT];
      target_colors[i]  = cluster_colors[i];
      old_colors[i]     = cluster_colors[i];
    }
}

static void generate_new_state(const float cluster_radius, const size_t cluster_count, size_t k)
{
  SetRandomSeed(GetRandomValue(0, 10000));

  set.count = 0;
  float c   = cluster_radius * lamda;
  for(size_t i = 0; i < k; ++i) { cluster[i].count = 0; }

  for(size_t i = 0; i < k; ++i)
    {
      Vector3 center = {
        .x = Lerp(-c, c, (float)GetRandomValue(0, 100) / 100),
        .y = Lerp(-c, c, (float)GetRandomValue(0, 100) / 100),
        .z = Lerp(-c, c, (float)GetRandomValue(0, 100) / 100),
      };
      generate_cluster(center, c, N * (float)GetRandomValue(50, 100) / 100, &set);
    }
}

static void reset_set(Samples3D *s)
{
  if(s->items) free(s->items);
  s->items    = NULL;
  s->count    = 0;
  s->capacity = 0;
}

static void append_to_cluster(Samples3D *s, Vector3 p)
{
  if(s->capacity == 0)
    {
      s->capacity = 1;
      s->items    = malloc(s->capacity * sizeof(Vector3));
    }

  if(s->count == s->capacity)
    {
      s->capacity *= 2;
      s->items = realloc(s->items, s->capacity * sizeof(Vector3));
    }
  s->items[s->count] = p;
  s->count++;
}

static void recluster_state(size_t kl)
{
  for(int i = 0; i < kl; ++i) { cluster[i].count = 0; }

  for(size_t i = 0; i < set.count; ++i)
    {
      Vector3 p = set.items[i];
      int     k = -1;
      float   s = FLT_MAX;
      for(size_t j = 0; j < kl; ++j)
        {
          Vector3 m  = target_means[j];
          float   sm = Vector3Distance(p, m);
          if(sm < s)
            {
              s = sm;
              k = j;
            }
        }
      append_to_cluster(&cluster[k], p);
    }
}

static void update_means(float cluster_radius, size_t cluster_count, size_t k)
{
  SetRandomSeed(GetRandomValue(0, 10000));
  for(size_t i = 0; i < k; ++i)
    {
      old_means[i]  = means[i];          // Store old means for animation
      old_colors[i] = cluster_colors[i]; // Store old colors for animation
      if(cluster[i].count > 0)
        {
          target_means[i] = Vector3Zero();
          for(size_t j = 0; j < cluster[i].count; ++j) { target_means[i] = Vector3Add(target_means[i], cluster[i].items[j]); }
          target_means[i].x /= cluster[i].count;
          target_means[i].y /= cluster[i].count;
          target_means[i].z /= cluster[i].count;
          target_colors[i] = colors[i % COLORS_COUNT]; // Assign new color
        }
      else
        {
          target_means[i].x = Lerp(-cluster_radius * lamda, cluster_radius * lamda, (float)GetRandomValue(0, 100) / 100);
          target_means[i].y = Lerp(-cluster_radius * lamda, cluster_radius * lamda, (float)GetRandomValue(0, 100) / 100);
          target_means[i].z = Lerp(-cluster_radius * lamda, cluster_radius * lamda, (float)GetRandomValue(0, 100) / 100);
          target_colors[i]  = colors[i % COLORS_COUNT]; // Assign new color
        }
    }
  animation_time = 0.0f; // Reset animation time
}

int main()
{
  const int   screenWidth    = 1920;
  const int   screenHeight   = 1080;
  const char *title          = "K-means Clustering Visualization";
  float       cluster_radius = 20;

  int k = K;

  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_FULLSCREEN_MODE );

  InitWindow(screenWidth, screenHeight, title);

  float camera_magnitude     = cluster_radius * lamda * 4;
  float camera_magnitude_vel = 0.0f;
  float camera_theta         = 0.5;
  float camera_phi           = 0.5;

  Camera3D camera = {
    .position =
      {
                 .x = sinf(camera_theta) * cosf(camera_phi) * camera_magnitude,
                 .y = sinf(camera_phi) * sinf(camera_phi) * camera_magnitude,
                 .z = cosf(camera_theta) * camera_magnitude,
                 },
    .target     = {0, 0, 0},
    .up         = {0, 1, 0},
    .fovy       = 90,
    .projection = CAMERA_PERSPECTIVE
  };

  bool isKMeansAnimation = false;
  SetTargetFPS(120);

  while(!WindowShouldClose())
    {
      if(IsKeyPressed(KEY_Q))
        {
          k += 1;
          if(isKMeansAnimation) isKMeansAnimation = false;
          if(k > K_MAX) k = K_MAX;
          recluster_state(k);
        }

      if(IsKeyPressed(KEY_W))
        {
          randomize_means(k, cluster_radius * lamda);
          recluster_state(k);
        }

      if(IsKeyPressed(KEY_A))
        {
          k -= 1;
          if(isKMeansAnimation) isKMeansAnimation = false;
          if(k < 1) k = 1;
          recluster_state(k);
        }

      if(IsKeyPressed(KEY_B))
        {
          centroid_selected       = false;
          selected_centroid_index = -1;
          camera.target           = Vector3Zero();
        }

      if(IsKeyPressed(KEY_R))
        {
          generate_new_state(cluster_radius, N, k);
          recluster_state(k);
        }

      if(IsKeyPressed(KEY_N))
        {
          centroid_selected       = true;
          selected_centroid_index = (selected_centroid_index + 1) % k;
          camera_start_target     = camera.target;
          camera_end_target       = means[selected_centroid_index];
          camera_transition       = true;
          camera_transition_time  = 0.0f;
          camera_start_pos        = camera.position;
          camera_end_pos          = (Vector3){
                     .x = means[selected_centroid_index].x + sinf(camera_theta) * cosf(camera_phi) * camera_magnitude,
                     .y = means[selected_centroid_index].y + sinf(camera_phi) * camera_magnitude,
                     .z = means[selected_centroid_index].z + cosf(camera_theta) * cosf(camera_phi) * camera_magnitude,
          };
        }

      if(IsKeyPressed(KEY_SPACE)) { isKMeansAnimation = !isKMeansAnimation; }

      float deltaTime = GetFrameTime();

      animation_time += deltaTime;

      if(animation_time > ANIMATION_DURATION) animation_time = ANIMATION_DURATION;

      for(size_t i = 0; i < k; ++i)
        {
          means[i]          = Vector3Lerp(old_means[i], target_means[i], animation_time / ANIMATION_DURATION);
          cluster_colors[i] = ColorAlpha(target_colors[i], 0.9f);
        }

      if(IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
          HideCursor();
          Vector2 delta = GetMouseDelta();
          camera_theta += delta.x * 0.01;
          camera_phi -= delta.y * 0.01;
          camera.position.x = camera.target.x + sinf(camera_theta) * cosf(camera_phi) * camera_magnitude;
          camera.position.y = camera.target.y + sinf(camera_phi) * camera_magnitude;
          camera.position.z = camera.target.z + cosf(camera_theta) * cosf(camera_phi) * camera_magnitude;
        }

      if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) ShowCursor();

      camera_magnitude += camera_magnitude_vel * deltaTime;

      if(camera_magnitude < 0) camera_magnitude = 0;

      camera_magnitude_vel -= GetMouseWheelMove() * SAMPLE_SIZE * 5 * k;
      camera_magnitude_vel *= 0.9f;

      camera.position.x = camera.target.x + sinf(camera_theta) * cosf(camera_phi) * camera_magnitude;
      camera.position.y = camera.target.y + sinf(camera_phi) * camera_magnitude;
      camera.position.z = camera.target.z + cosf(camera_theta) * cosf(camera_phi) * camera_magnitude;

      if(camera_transition)
        {
          camera_transition_time += deltaTime;
          if(camera_transition_time > CAMERA_TRANSITION_DURATION)
            {
              camera_transition_time = CAMERA_TRANSITION_DURATION;
              camera_transition      = false;
            }
          float t         = camera_transition_time / CAMERA_TRANSITION_DURATION;
          camera.position = Vector3Lerp(camera_start_pos, camera_end_pos, t);
          camera.target   = Vector3Lerp(camera_start_target, camera_end_target, t);
        }

      if(isKMeansAnimation)
        {
          update_means(cluster_radius, N, k);
          recluster_state(k);
          camera_start_target = camera.target;
          camera_end_target   = means[selected_centroid_index];
          camera_start_pos    = camera.position;
          camera_end_pos      = (Vector3){
                 .x = means[selected_centroid_index].x + sinf(camera_theta) * cosf(camera_phi) * camera_magnitude,
                 .y = means[selected_centroid_index].y + sinf(camera_phi) * camera_magnitude,
                 .z = means[selected_centroid_index].z + cosf(camera_theta) * cosf(camera_phi) * camera_magnitude,
          };
          camera_transition = true;
        }

      BeginDrawing();
      ClearBackground(BLACK);
      BeginMode3D(camera);

      for(size_t i = 0; i < k; i++)
        {
          for(size_t j = 0; j < cluster[i].count; j++)
            {
              Vector3 p = cluster[i].items[j];
              if(i == selected_centroid_index)
                {
                  int size_boost = 4;
                  DrawCube(p, SAMPLE_SIZE * size_boost, SAMPLE_SIZE * size_boost, SAMPLE_SIZE * size_boost, cluster_colors[i]);
                  DrawCubeWires(p, SAMPLE_SIZE * size_boost, SAMPLE_SIZE * size_boost, SAMPLE_SIZE * size_boost,
                                ColorAlphaBlend(cluster_colors[i], BLACK, WHITE));
                }
              else
                DrawCubeWires(p, SAMPLE_SIZE, SAMPLE_SIZE, SAMPLE_SIZE, cluster_colors[i]);
            }
          if(i == selected_centroid_index)
            {
              DrawSphere(means[i], MEAN_SIZE * 4, WHITE);
              DrawSphere(means[i], cluster_radius * lamda, ColorAlpha(WHITE, 0.25f));
            }
          else
            DrawSphere(means[i], MEAN_SIZE * 4, cluster_colors[i]);
        }
      if(!centroid_selected)
        {
          DrawBoundingBox(
            (BoundingBox){
              (Vector3){-cluster_radius * lamda * 2, -cluster_radius * lamda * 2, -cluster_radius * lamda * 2},
              (Vector3){cluster_radius * lamda * 2,  cluster_radius * lamda * 2,  cluster_radius * lamda * 2 }
          },
            WHITE);
        }

      EndMode3D();
      if(isKMeansAnimation) { DrawText("Press [SPACE] to pause Kmeans ", 10, 10, 20, WHITE); }
      else { DrawText("Press [SPACE] to start Kmeans Clustering ", 10, 10, 20, WHITE); }
      DrawText("Press [R] to generate new data", 10, 35, 20, WHITE);
      if(centroid_selected && selected_centroid_index != -1)
        {
          DrawText("Selected Centroid - press B to unselect ", -10, -70, 20, cluster_colors[selected_centroid_index]);
        }
      EndDrawing();
      DrawText(TextFormat("K = %d press [A] for k - 1  or [Q] for k + 1", k), 10, 60, 10, WHITE);
      DrawText("Press [W] to randomize means", 10, 80, 10, WHITE);
      DrawText("Press [N] to select next centroid", 10, 100, 10, cluster_colors[selected_centroid_index]);
      DrawText("Press [B] to unselect centroid", 10, 120, 10, WHITE);
      DrawText("Use mouse to rotate camera", 10, 140, 10, WHITE);
      DrawText("Use mouse wheel to zoom in/out", 10, 160, 10, WHITE);
      DrawFPS(10, 180);
    }

  CloseWindow();
  return 0;
}
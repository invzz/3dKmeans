#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <raylib.h>
#include "main.h" // Ensure main.h contains necessary declarations

#define SAMPLE_SIZE  0.5
#define MEAN_SIZE    SAMPLE_SIZE * 2
#define SAMPLE_COLOR RED
#define CAMERA_SPEED SAMPLE_SIZE * 2
#define N            5000
#define K            12
#define COLORS_COUNT (sizeof(colors) / sizeof(colors[0]))

// Colors array for clusters
static Color colors[] = {RED, GREEN, BLUE, YELLOW, ORANGE, VIOLET, BROWN, LIGHTGRAY, PINK, GOLD, MAGENTA, DARKGREEN, DARKPURPLE, LIME, BROWN, DARKBLUE, DARKBROWN, DARKGRAY, MAROON, PURPLE, RAYWHITE};

typedef struct
{
  Vector3 *items;
  size_t   count;
  size_t   capacity;
} Samples3D;

static Samples3D   set                     = {0};
static Samples3D   cluster[K]              = {0};
static Vector3     means[K]                = {0};
static Vector3     old_means[K]            = {0};
static Vector3     target_means[K]         = {0};
static Color       cluster_colors[K]       = {0};
static Color       old_colors[K]           = {0};
static Color       target_colors[K]        = {0};
static float       animation_time          = 0.0f;
static const float ANIMATION_DURATION      = 0.5f;
static bool        centroid_selected       = false;
static int         selected_centroid_index = -1;

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

static void generate_new_state(const float cluster_radius, const size_t cluster_count)
{
  SetRandomSeed(GetRandomValue(0, 10000));

  set.count = 0;
  for(size_t i = 0; i < K; ++i) { cluster[i].count = 0; }

  for(size_t i = 0; i < K; ++i)
    {
      Vector3 center = {
        .x = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100),
        .y = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100),
        .z = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100),
      };
      generate_cluster(center, cluster_radius, N / K, &set);
    }

  for(size_t i = 0; i < K; ++i)
    {
      means[i].x        = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100);
      means[i].y        = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100);
      means[i].z        = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100);
      target_means[i]   = means[i];
      old_means[i]      = means[i];
      cluster_colors[i] = colors[i % COLORS_COUNT];
      target_colors[i]  = cluster_colors[i];
      old_colors[i]     = cluster_colors[i];
    }
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

static void recluster_state(void)
{
  for(int i = 0; i < K; ++i) { cluster[i].count = 0; }

  for(size_t i = 0; i < set.count; ++i)
    {
      Vector3 p = set.items[i];
      int     k = -1;
      float   s = FLT_MAX;
      for(size_t j = 0; j < K; ++j)
        {
          Vector3 m  = target_means[j];
          float   sm = Vector3LengthSqr(Vector3Subtract(p, m));
          if(sm < s)
            {
              s = sm;
              k = j;
            }
        }
      append_to_cluster(&cluster[k], p);
    }
}

static void update_means(float cluster_radius, size_t cluster_count)
{
  SetRandomSeed(GetRandomValue(0, 10000));
  for(size_t i = 0; i < K; ++i)
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
          target_means[i].x = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100);
          target_means[i].y = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100);
          target_means[i].z = Lerp(-cluster_radius, cluster_radius, (float)GetRandomValue(0, 100) / 100);
          target_colors[i]  = colors[i % COLORS_COUNT]; // Assign new color
        }
    }
  animation_time = 0.0f; // Reset animation time
}

static bool RayIntersectsSphere(Ray ray, Vector3 sphereCenter, float sphereRadius, float *outDistance)
{
  Vector3 rayToCenter = Vector3Subtract(sphereCenter, ray.position);
  float   tca         = Vector3DotProduct(rayToCenter, ray.direction);
  float   d2          = Vector3DotProduct(rayToCenter, rayToCenter) - tca * tca;
  float   radius2     = sphereRadius * sphereRadius;
  if(d2 > radius2) return false;
  float thc    = sqrtf(radius2 - d2);
  *outDistance = tca - thc;
  float t1     = tca + thc;
  if(*outDistance < 0) *outDistance = t1;
  return *outDistance >= 0;
}

int main()
{
  const int   screenWidth    = 800;
  const int   screenHeight   = 450;
  const char *title          = "K-means Clustering Visualization";
  float       cluster_radius = 50;

  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(screenWidth, screenHeight, title);

  float camera_magnitude     = SAMPLE_SIZE * 10 * K;
  float camera_magnitude_vel = 0.0f;
  float camera_theta         = 0.0;
  float camera_phi           = 0.0;

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

  SetTargetFPS(60);

  while(!WindowShouldClose())
    {
      if(IsKeyPressed(KEY_B))
        {
          centroid_selected       = false;
          selected_centroid_index = -1;
          camera.target           = Vector3Zero();
        }

      if(IsKeyPressed(KEY_R))
        {
          generate_new_state(cluster_radius, N);
          recluster_state();
        }

      if(IsKeyPressed(KEY_SPACE))
        {
          update_means(cluster_radius, N);
          recluster_state();
        }

      float deltaTime = GetFrameTime();
      animation_time += deltaTime;
      if(animation_time > ANIMATION_DURATION) animation_time = ANIMATION_DURATION;

      for(size_t i = 0; i < K; ++i)
        {
          means[i]          = Vector3Lerp(old_means[i], target_means[i], animation_time / ANIMATION_DURATION);
          cluster_colors[i] = ColorAlpha(target_colors[i], 0.9f);
        }

      if(IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
          Vector2 delta = GetMouseDelta();
          camera_theta += delta.x * 0.01; // Adjust rotation around Y axis based on horizontal mouse movement
          camera_phi -= delta.y * 0.01;   // Adjust rotation around X axis based on vertical mouse movement

          // Clamp camera_phi to avoid flipping upside down
          if(camera_phi > PI / 2.0f) camera_phi = PI / 2.0f;
          if(camera_phi < -PI / 2.0f) camera_phi = -PI / 2.0f;

          // Update camera position based on new orientation
          camera.position.x = sinf(camera_theta) * cosf(camera_phi) * camera_magnitude;
          camera.position.y = sinf(camera_phi) * camera_magnitude;
          camera.position.z = cosf(camera_theta) * cosf(camera_phi) * camera_magnitude;
        }
      camera_magnitude += camera_magnitude_vel * deltaTime;
      if(camera_magnitude < 0) camera_magnitude = 0;

      camera_magnitude_vel -= GetMouseWheelMove() * SAMPLE_SIZE * 5 * K;
      camera_magnitude_vel *= 0.9f;

      Vector3 camera_position = {
        .x = sinf(camera_theta) * cosf(camera_phi) * camera_magnitude,
        .y = sinf(camera_phi) * sinf(camera_phi) * camera_magnitude,
        .z = cosf(camera_theta) * camera_magnitude,
      };

      camera.position = camera_position;

      // Check if a centroid is clicked
      if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
          Vector2 mousePos = GetMousePosition();
          Ray     ray      = GetMouseRay(mousePos, camera);
          for(size_t i = 0; i < K; ++i)
            {
              float distance;
              if(RayIntersectsSphere(ray, means[i], MEAN_SIZE, &distance))
                {
                  centroid_selected       = true;
                  selected_centroid_index = i;
                  camera.target           = means[i];
                  break;
                }
            }
        }

      // Check if a centroid is clicked

      if(centroid_selected && selected_centroid_index != -1)
        {
          // Update camera target to follow the selected centroid
          camera.target = means[selected_centroid_index];
        }

      BeginDrawing();

      ClearBackground(ColorAlpha(BLACK, 0.9f));

      BeginMode3D(camera);

      for(size_t i = 0; i < K; i++)
        {
          for(size_t j = 0; j < cluster[i].count; j++)
            {
              Vector3 p = cluster[i].items[j];
              DrawCube(p, SAMPLE_SIZE, SAMPLE_SIZE, SAMPLE_SIZE, cluster_colors[i]);
            }
          DrawSphere(means[i], MEAN_SIZE * 2, i == selected_centroid_index? WHITE : cluster_colors[i]);
        }

      EndMode3D();

      DrawText("Press [SPACE] to update means", 10, 10, 20, WHITE);

      DrawText("Press [R] to generate new data", 10, 35, 20, WHITE);

      if(centroid_selected && selected_centroid_index != -1)
        {
          DrawText("Selected Centroid - press B to unselect ", 10, 70, 20, cluster_colors[selected_centroid_index]);
        }

      EndDrawing();
    }
  CloseWindow();
  return 0;
}
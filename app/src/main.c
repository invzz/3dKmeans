#include "main.h" // Ensure main.h contains necessary declarations
#include "common.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#ifdef USE_NVIDIA_CARD
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
#endif

#define MAIN_BACKGROUND_COLOR                                                                                                                                  \
  (Color) { 20, 20, 20, 255 }

static Samples3D set                     = {0};
static Samples3D cluster[K_MAX]          = {0};
static Vector3   means[K_MAX]            = {0};
static Vector3   old_means[K_MAX]        = {0};
static Vector3   target_means[K_MAX]     = {0};
static Vector3   camera_start_pos        = {0};
static Vector3   camera_end_pos          = {0};
static Vector3   camera_start_target     = {0};
static Vector3   camera_end_target       = {0};
static Vector3   light                   = {0};
static Color     cluster_colors[K_MAX]   = {0};
static Color     old_colors[K_MAX]       = {0};
static Color     target_colors[K_MAX]    = {0};
static bool      camera_transition       = false;
static bool      centroid_selected       = false;
static bool      isKMeansAnimation       = false;
static float     cube_rotation_angle     = 0.0f;
static size_t    current_k               = 0;
static int       selected_centroid_index = -1;

static const float ANIMATION_DURATION         = 0.5f;
static float       animation_time             = 0.0f;
static float       camera_transition_time     = 0.0f;
static float       camera_transition_duration = 2.0f;
static float       camera_magnitude_vel       = 0.0f;
static float       camera_theta               = 0.5;
static float       camera_phi                 = 0.5;

static int k = INITIAL_K;

float       light_rotation_angle = 0.0f; // Initial light rotation angle
const float light_rotation_speed = 1.0f; // Light rotation speed

static Camera3D camera = {0};

void InitCamera(const float camera_magnitude);
void EventHandler(float *cluster_radius, float *camera_magnitude);
void UpdateCameraPosition(float *cluster_radius, float *camera_magnitude);
void DoAnimations(const float cluster_radius);
void DrawAxis(Vector3 center, float length);

int main()
{
  const int   screenWidth      = 1280;
  const int   screenHeight     = 720;
  const char *title            = "K-means Clustering Visualization";
  float       cluster_radius   = 50;
  float       camera_magnitude = cluster_radius * 5;

  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_MAXIMIZED);

  InitWindow(screenWidth, screenHeight, title);

  InitCamera(camera_magnitude);

  randomize_means(k, cluster_radius * 2);

  // Speed of light rotation

  while(!WindowShouldClose())
    {
      ClearBackground(MAIN_BACKGROUND_COLOR);
      EventHandler(&cluster_radius, &camera_magnitude);
      UpdateCameraPosition(&cluster_radius, &camera_magnitude);
      DoAnimations(cluster_radius);
      BeginDrawing();

      BeginMode3D(camera);

      for(size_t i = 0; i < current_k; i++)
        {
          for(size_t j = 0; j < cluster[i].count; j++)
            {
              Vector3 p = cluster[i].items[j];
              Vector3 s = {2, 2, 2};
              if(i == selected_centroid_index) { DrawCubeV(p, s, ColorAlpha(cluster_colors[i], 1)); }
              else { DrawCubeV(p, s, cluster_colors[i]); }
            }
          if(i == selected_centroid_index) { DrawSphere(means[i], MEAN_SIZE * 4, WHITE); }
          else { DrawSphere(means[i], MEAN_SIZE * 4, ColorAlpha(cluster_colors[i], 1)); }
        }

      DrawAxis(Vector3Zero(), cluster_radius);

      DrawSphere(light, 3, YELLOW);

      DrawBoundingBox(
        (BoundingBox){
          (Vector3){-cluster_radius * 2, -cluster_radius * 2, -cluster_radius * 2},
          (Vector3){cluster_radius * 2,  cluster_radius * 2,  cluster_radius * 2 }
      },
        WHITE);

      EndMode3D();

      int current_text_y = 10;
      int text_size      = 10;
      int text_padding   = 10;

      if(isKMeansAnimation) { DrawText("Press [SPACE] to pause Kmeans ", 10, current_text_y, text_size, WHITE); }

      else { DrawText("Press [SPACE] to start Kmeans Clustering ", 10, current_text_y, text_size, WHITE); }

      current_text_y += text_size + text_padding;

      DrawText("Press [R] to generate new data", 10, current_text_y, text_size, WHITE);

      current_text_y += text_size + text_padding;

      if(centroid_selected && selected_centroid_index != -1)
        {
          DrawText("Selected Centroid - press B to unselect ", -10, -70, 20, cluster_colors[selected_centroid_index]);
        }
      current_text_y += text_size + text_padding;
      EndDrawing();
      DrawText(TextFormat("K = %d press [A] for k - 1  or [Q] for k + 1", k), 10, current_text_y, text_size, WHITE);
      current_text_y += text_size + text_padding;
      DrawText("Press [W] to randomize means", 10, current_text_y, text_size, WHITE);
      current_text_y += text_size + text_padding;
      DrawText("Press [N] to select next centroid", 10, current_text_y, text_size,
               centroid_selected ? ColorAlpha(cluster_colors[selected_centroid_index], 1) : WHITE);
      current_text_y += text_size + text_padding;
      DrawText("Press [B] to unselect centroid", 10, current_text_y, text_size, WHITE);
      current_text_y += text_size + text_padding;
      DrawText("Press [G] to increase cluster radius", 10, current_text_y, text_size, WHITE);
      current_text_y += text_size + text_padding;
      DrawText("Press [H] to decrease cluster radius", 10, current_text_y, text_size, WHITE);
      current_text_y += text_size + text_padding;
      DrawText("Use mouse to rotate camera", 10, current_text_y, text_size, WHITE);
      current_text_y += text_size + text_padding;
      DrawText("Use mouse wheel to zoom in/out", 10, current_text_y, text_size, WHITE);
      current_text_y += text_size + text_padding;
      DrawFPS(10, screenHeight - 20);
    }
  CloseWindow();
  return 0;
}

void InitCamera(const float camera_magnitude)
{
  camera.position =
    (Vector3){
      .x = sinf(camera_theta) * cosf(camera_phi) * camera_magnitude,
      .y = sinf(camera_phi) * sinf(camera_phi) * camera_magnitude,
      .z = cosf(camera_theta) * camera_magnitude,
    },
  camera.target     = (Vector3){0, 0, 0};
  camera.up         = (Vector3){0, 1, 0};
  camera.fovy       = 90;
  camera.projection = CAMERA_PERSPECTIVE;
}

void EventHandler(float *cluster_radius, float *camera_magnitude)
{
  // Handle key inputs for updating the cluster and camera settings
  if(IsKeyPressed(KEY_G)) { *cluster_radius += 1; }
  if(IsKeyPressed(KEY_H))
    {
      *cluster_radius -= 1;
      if(*cluster_radius < 10) *cluster_radius = 10;
    }
  if(IsKeyPressed(KEY_Q))
    {
      k += 1;
      // if(isKMeansAnimation) isKMeansAnimation = false;
      if(k > K_MAX) k = K_MAX;
      randomize_means(k, *cluster_radius * 2);
    }
  if(IsKeyPressed(KEY_W))
    {
      randomize_means(k, *cluster_radius * 2);
      recluster_state(k);
    }
  if(IsKeyPressed(KEY_A))
    {
      k -= 1;
      // if(isKMeansAnimation) isKMeansAnimation = false;
      if(k < 1) k = 1;
      randomize_means(k, *cluster_radius * 2);
    }
  if(IsKeyPressed(KEY_B))
    {
      centroid_selected       = false;
      selected_centroid_index = -1;
      camera.target           = Vector3Zero();
    }
  if(IsKeyPressed(KEY_R))
    {
      generate_data(*cluster_radius, k);
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
                 .x = means[selected_centroid_index].x + sinf(camera_theta) * cosf(camera_phi) * *camera_magnitude,
                 .y = means[selected_centroid_index].y + sinf(camera_phi) * *camera_magnitude,
                 .z = means[selected_centroid_index].z + cosf(camera_theta) * cosf(camera_phi) * *camera_magnitude,
      };
      light = means[selected_centroid_index];
    }
  if(IsKeyPressed(KEY_SPACE)) { isKMeansAnimation = !isKMeansAnimation; }
  if(IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
      HideCursor();
      Vector2 delta = GetMouseDelta();
      camera_theta += delta.x * 0.01;
      camera_phi -= delta.y * 0.01;
      camera.position.x = camera.target.x + sinf(camera_theta) * cosf(camera_phi) * *camera_magnitude;
      camera.position.y = camera.target.y + sinf(camera_phi) * *camera_magnitude;
      camera.position.z = camera.target.z + cosf(camera_theta) * cosf(camera_phi) * *camera_magnitude;
    }
  if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) ShowCursor();
}

void UpdateCameraPosition(float *cluster_radius, float *camera_magnitude)
{
  float deltaTime = GetFrameTime();
  *camera_magnitude += camera_magnitude_vel * deltaTime;
  if(*camera_magnitude < 0) *camera_magnitude = 0;
  camera_magnitude_vel -= GetMouseWheelMove() * SAMPLE_SIZE * 5 * k;
  camera_magnitude_vel *= 0.9f;
  camera.position.x = camera.target.x + sinf(camera_theta) * cosf(camera_phi) * *camera_magnitude;
  camera.position.y = camera.target.y + sinf(camera_phi) * *camera_magnitude;
  camera.position.z = camera.target.z + cosf(camera_theta) * cosf(camera_phi) * *camera_magnitude;

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
      update_means(*cluster_radius, k);
      recluster_state(k);
      camera_start_target = camera.target;
      camera_end_target   = means[selected_centroid_index];
      camera_start_pos    = camera.position;
      camera_end_pos      = (Vector3){
             .x = means[selected_centroid_index].x + sinf(camera_theta) * cosf(camera_phi) * *camera_magnitude,
             .y = means[selected_centroid_index].y + sinf(camera_phi) * *camera_magnitude,
             .z = means[selected_centroid_index].z + cosf(camera_theta) * cosf(camera_phi) * *camera_magnitude,
      };
      camera_transition = true;
    }
}

void DoAnimations(const float cluster_radius)
{
  float deltaTime = GetFrameTime();
  animation_time += deltaTime;
  if(animation_time > ANIMATION_DURATION) animation_time = ANIMATION_DURATION;
  for(size_t i = 0; i < k; ++i)
    {
      means[i] = Vector3Lerp(old_means[i], target_means[i], animation_time / ANIMATION_DURATION);
      // cluster_colors[i] = ColorAlpha(target_colors[i], 0.9f);
    }
  // Update light position to rotate around its target
  light_rotation_angle += light_rotation_speed * deltaTime * 10;
  light.x = camera.target.x + (cluster_radius / 2) * cosf(light_rotation_angle);
  light.y = camera.target.y + (cluster_radius / 2) * sinf(light_rotation_angle);
  light.z = camera.target.z + (cluster_radius / 2) * sinf(light_rotation_angle * 0.5f); // Different axis rotation
}

void DrawAxis(Vector3 center, float length)
{
  DrawLine3D(center, (Vector3){center.x + length, center.y, center.z}, RED);
  DrawLine3D(center, (Vector3){center.x, center.y + length, center.z}, GREEN);
  DrawLine3D(center, (Vector3){center.x, center.y, center.z + length}, BLUE);
}

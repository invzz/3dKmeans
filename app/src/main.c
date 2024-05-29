#include "main.h" // Ensure main.h contains necessary declarations
#include "common.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

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
static Vector3     camera_start_pos           = {0};
static Vector3     camera_end_pos             = {0};
static Vector3     camera_start_target        = {0};
static Vector3     camera_end_target          = {0};
static bool        camera_transition          = false;
static float       camera_transition_time     = 0.0f;
static float       camera_transition_duration = 2.0f;
static size_t      current_k                  = 0;

static bool isKMeansAnimation = false;

int main()
{
  const int   screenWidth    = 800;
  const int   screenHeight   = 600;
  const char *title          = "K-means Clustering Visualization";
  float       cluster_radius = 50;

  int k = K;

  SetConfigFlags(FLAG_MSAA_4X_HINT);

  InitWindow(screenWidth, screenHeight, title);

  // Load model
  Model model = LoadModelFromMesh(GenMeshCube(1, 1, 1));

  // texture the model
  Texture texture                                       = LoadTexture(TEXTURE_DIR "gold/ao.png");
  model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  // shader
  Shader shader                        = LoadShader(SHADER_DIR "light.vs", SHADER_DIR "light.fs");
  shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
  shader.locs[SHADER_LOC_VECTOR_VIEW]  = GetShaderLocation(shader, "viewPos");

  // ambient light level
  int amb = GetShaderLocation(shader, "ambient");
  SetShaderValue(shader, amb, (float[4]){0.2, 0.2, 0.2, 1.0}, SHADER_UNIFORM_VEC4);

  // set the models shader
  model.materials[0].shader = shader;

  // make a light
  Light light = CreateLight(LIGHT_DIRECTIONAL, (Vector3){2, 2, 0}, Vector3Zero(), WHITE, shader);

  float camera_magnitude     = cluster_radius * 4;
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

  SetTargetFPS(120);

  float       light_rotation_angle = 0.0f; // Initial light rotation angle
  const float light_rotation_speed = 1.0f; // Speed of light rotation

  while(!WindowShouldClose())
    {
      // Handle key inputs for updating the cluster and camera settings
      if(IsKeyPressed(KEY_G)) { cluster_radius += 1; }
      if(IsKeyPressed(KEY_H))
        {
          cluster_radius -= 1;
          if(cluster_radius < 10) cluster_radius = 10;
        }
      if(IsKeyPressed(KEY_Q))
        {
          k += 1;
          if(isKMeansAnimation) isKMeansAnimation = false;
          if(k > K_MAX) k = K_MAX;
          recluster_state(k);
        }
      if(IsKeyPressed(KEY_W))
        {
          randomize_means(k, cluster_radius * 2);
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
          generate_new_state(cluster_radius, k);
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
          light.position = means[selected_centroid_index];
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

      // Update light position to rotate around its target
      light_rotation_angle += light_rotation_speed * deltaTime * 10;
      light.position.x = camera.target.x + (cluster_radius / 2) * cosf(light_rotation_angle);
      light.position.y = camera.target.y + (cluster_radius / 2) * sinf(light_rotation_angle);
      light.position.z = camera.target.z + (cluster_radius / 2) * sinf(light_rotation_angle * 0.5f); // Different axis rotation

      UpdateLightValues(shader, light);

      BeginDrawing();
      ClearBackground(BLACK);
      BeginMode3D(camera);

      for(size_t i = 0; i < current_k; i++)
        {
          for(size_t j = 0; j < cluster[i].count; j++)
            {
              Vector3 p = cluster[i].items[j];
              if(i == selected_centroid_index)
                {
                  int size_boost = 4;
                  DrawCube(p, 3, 3, 3, cluster_colors[i]);
                  DrawCubeWires(p, SAMPLE_SIZE * size_boost, SAMPLE_SIZE * size_boost, SAMPLE_SIZE * size_boost,
                                ColorAlphaBlend(cluster_colors[i], BLACK, WHITE));
                }
              else
                {
                  model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = cluster_colors[i];
                  DrawCube(p, 3, 3, 3, cluster_colors[i]);
                }
            }
          if(i == selected_centroid_index)
            {
              DrawSphere(means[i], MEAN_SIZE * 4, WHITE);
              // DrawSphere(means[i], cluster_radius, ColorAlpha(WHITE, 0.25f));
            }
          else { DrawSphere(means[i], MEAN_SIZE * 4, cluster_colors[i]); }
        }

      DrawBoundingBox(
        (BoundingBox){
          (Vector3){-cluster_radius * 2, -cluster_radius * 2, -cluster_radius * 2},
          (Vector3){cluster_radius * 2,  cluster_radius * 2,  cluster_radius * 2 }
      },
        WHITE);

      DrawSphere(light.position, 4.0, WHITE);

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

/*******************************************************************************************
 *
 *   raylib [models] example - simple lighting material
 *
 *   This example has been created using raylib 2.5 (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
 *
 *   This example Copyright (c) 2018 Chris Camacho (codifies) http://bedroomcoders.co.uk/captcha/
 *
 * THIS EXAMPLE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This example may be freely redistributed.
 *
 ********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

#include <stdio.h>

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

/*
 *
 * This is based on the PBR lighting example, but greatly simplified to aid learning...
 * actually there is very little of the PBR example left!
 *
 * When I first looked at the bewildering complexity of the PBR example I feared
 * I would never understand how I could do simple lighting with raylib however its
 * a testement to the authors of raylib (including rlights.h) that the example
 * came together fairly quickly.
 *
 * I'm no expert at light calculation in shaders, so really probably a fudge!
 *
 */

int main(void)
{
  // Initialization
  //--------------------------------------------------------------------------------------
  const int screenWidth  = 1280;
  const int screenHeight = 720;

  SetConfigFlags(FLAG_MSAA_4X_HINT); // Enable Multi Sampling Anti Aliasing 4x (if available)
  InitWindow(screenWidth, screenHeight, "raylib [models] example - simple lighting material");

  // Define the camera to look into our 3d world
  Camera camera = {
    (Vector3){2.0f, 2.0f, 6.0f},
    (Vector3){0.0f, 0.5f, 0.0f},
    (Vector3){0.0f, 1.0f, 0.0f},
    45.0f, CAMERA_PERSPECTIVE
  };

  // Load models
  Model model  = LoadModelFromMesh(GenMeshTorus(.4, 1, 16, 32));
  Model model2 = LoadModelFromMesh(GenMeshCube(1, 1, 1));
  Model model3 = LoadModelFromMesh(GenMeshSphere(.5, 32, 32));

  // texture the models
  Texture texture = LoadTexture("data/test.png");

  model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture  = texture;
  model2.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
  model3.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  Shader shader = LoadShader("data/simpleLight.vs", "data/simpleLight.fs");
  // load a shader into the first model and set up some uniforms
  shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
  shader.locs[SHADER_LOC_VECTOR_VIEW]  = GetShaderLocation(shader, "viewPos");

  // ambient light level
  int amb = GetShaderLocation(shader, "ambient");
  SetShaderValue(shader, amb, (float[4]){0.2, 0.2, 0.2, 1.0}, SHADER_UNIFORM_VEC4);

  // models 2 & 3 share the first models shader
  model.materials[0].shader  = shader;
  model2.materials[0].shader = shader;
  model3.materials[0].shader = shader;

  // using 4 point lights, white, red, green and blue
  Light lights[MAX_LIGHTS];
  lights[0] = CreateLight(LIGHT_POINT, (Vector3){4, 2, 4}, Vector3Zero(), WHITE, shader);
  lights[1] = CreateLight(LIGHT_POINT, (Vector3){4, 2, 4}, Vector3Zero(), RED, shader);
  lights[2] = CreateLight(LIGHT_POINT, (Vector3){0, 4, 2}, Vector3Zero(), GREEN, shader);
  lights[3] = CreateLight(LIGHT_POINT, (Vector3){0, 4, 2}, Vector3Zero(), BLUE, shader);

  SetCameraMode(camera, CAMERA_ORBITAL); // Set an orbital camera mode

  SetTargetFPS(60); // Set our game to run at 60 frames-per-second
  //--------------------------------------------------------------------------------------
  float angle = 6.282;
  // Main game loop
  while(!WindowShouldClose()) // Detect window close button or ESC key
    {
      // Update
      //----------------------------------------------------------------------------------
      if(IsKeyPressed(KEY_W)) { lights[0].enabled = !lights[0].enabled; }
      if(IsKeyPressed(KEY_R)) { lights[1].enabled = !lights[1].enabled; }
      if(IsKeyPressed(KEY_G)) { lights[2].enabled = !lights[2].enabled; }
      if(IsKeyPressed(KEY_B)) { lights[3].enabled = !lights[3].enabled; }

      UpdateCamera(&camera, CAMERA_ORBITAL); // Update camera

      // make the lights do differing orbits
      angle -= 0.02;
      lights[0].position.x = cos(angle) * 4.0;
      lights[0].position.z = sin(angle) * 4.0;
      lights[1].position.x = cos(-angle * 0.6) * 4.0;
      lights[1].position.z = sin(-angle * 0.6) * 4.0;
      lights[2].position.y = cos(angle * .2) * 4.0;
      lights[2].position.z = sin(angle * .2) * 4.0;
      lights[3].position.y = cos(-angle * 0.35) * 4.0;
      lights[3].position.z = sin(-angle * 0.35) * 4.0;
      UpdateLightValues(shader, lights[0]);
      UpdateLightValues(shader, lights[1]);
      UpdateLightValues(shader, lights[2]);
      UpdateLightValues(shader, lights[3]);

      // rotate the torus
      model.transform = MatrixMultiply(model.transform, MatrixRotateX(-0.025));
      model.transform = MatrixMultiply(model.transform, MatrixRotateZ(0.012));

      // update the light shader with the camera view position
      SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position.x, SHADER_UNIFORM_VEC3);
      //----------------------------------------------------------------------------------

      // Draw
      //----------------------------------------------------------------------------------
      BeginDrawing();

      ClearBackground(BLACK);

      BeginMode3D(camera);

      // draw the three models
      DrawModel(model, Vector3Zero(), 1.0f, WHITE);
      DrawModel(model2, (Vector3){-1.6, 0, 0}, 1.0f, WHITE);
      DrawModel(model3, (Vector3){1.6, 0, 0}, 1.0f, WHITE);

      // draw markers to show where the lights are
      if(lights[0].enabled) { DrawCube(lights[0].position, .2, .2, .2, WHITE); }
      if(lights[1].enabled) { DrawCube(lights[1].position, .2, .2, .2, RED); }
      if(lights[2].enabled) { DrawCube(lights[2].position, .2, .2, .2, GREEN); }
      if(lights[3].enabled) { DrawCube(lights[3].position, .2, .2, .2, BLUE); }

      DrawGrid(10, 1.0f);

      EndMode3D();

      DrawFPS(10, 10);
      DrawText("Keys RGB & W toggle lights", 10, 30, 20, WHITE);

      EndDrawing();
      //----------------------------------------------------------------------------------
    }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  UnloadModel(model); // Unload the model
  UnloadModel(model2);
  UnloadModel(model3);
  UnloadTexture(texture); // Unload the texture
  UnloadShader(shader);

  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}

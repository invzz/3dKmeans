
#include "data_handler.h"

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

static void generate_new_state(const float cluster_radius, size_t k)
{
  SetRandomSeed(GetRandomValue(0, 10000));

  Vector3 centers[k];
  float   sphere_radius = cluster_radius * 2;
  generate_uniform_sphere_centers(sphere_radius, k, centers);
  set.count = 0;
  float c   = cluster_radius;
  for(size_t i = 0; i < k; ++i) { cluster[i].count = 0; }
  for(size_t i = 0; i < k; ++i) { generate_cluster(centers[i], c, N * (float)GetRandomValue(50, 100) / 100, &set); }
}

void generate_uniform_sphere_centers(float sphere_radius, size_t num_clusters, Vector3 *centers)
{
  for(size_t i = 0; i < num_clusters; ++i)
    {
      float theta = acosf(1.0f - 2.0f * ((float)rand() / RAND_MAX)); // Random inclination angle
      float phi   = 2.0f * M_PI * ((float)rand() / RAND_MAX);        // Random azimuthal angle

      // Convert spherical coordinates to Cartesian coordinates
      centers[i] = sphericalToCartesian(sphere_radius, theta, phi);
    }
}

Vector3 sphericalToCartesian(float radius, float theta, float phi)
{
  Vector3 v;
  v.x = radius * sinf(theta) * cosf(phi);
  v.y = radius * sinf(theta) * sinf(phi);
  v.z = radius * cosf(theta);
  return v;
}
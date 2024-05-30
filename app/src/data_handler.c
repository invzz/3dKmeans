
#include "data_handler.h"

// static void generate_cluster(Vector3 center, float radius, size_t count, Samples3D *samples)
// {
//   for(size_t i = 0; i < count; i++)
//     {
//       if(samples->capacity == 0)
//         {
//           samples->capacity = 1;
//           samples->items    = malloc(samples->capacity * sizeof(Vector3));
//         }
//       if(samples->count == samples->capacity)
//         {
//           samples->capacity *= 2;
//           samples->items = realloc(samples->items, samples->capacity * sizeof(Vector3));
//         }
//       float theta                    = GetRandomValue(0, 360) * DEG2RAD;
//       float phi                      = GetRandomValue(0, 360) * DEG2RAD;
//       float r                        = GetRandomValue(0, radius);
//       float x                        = center.x + (r * sinf(theta) * cosf(phi));
//       float y                        = center.y + (r * sinf(theta) * sinf(phi));
//       float z                        = center.z + (r * cosf(theta));
//       samples->items[samples->count] = (Vector3){x, y, z};
//       samples->count++;
//     }
// }
// Function to generate a normally distributed random number
float generate_normal_random(float mean, float stddev)
{
  float u1 = GetRandomValue(0, 1000) / 1000.0f;
  float u2 = GetRandomValue(0, 1000) / 1000.0f;
  float z0 = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * PI * u2);
  return z0 * stddev + mean;
}

static void generate_cluster(Vector3 center, float stddev, size_t count, Samples3D *samples)
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

      float theta = GetRandomValue(0, 360) * DEG2RAD;
      float phi   = GetRandomValue(0, 360) * DEG2RAD;
      float r     = fabs(generate_normal_random(0, stddev)); // Distance from the center, absolute value to ensure non-negative

      float x = center.x + (r * sinf(theta) * cosf(phi));
      float y = center.y + (r * sinf(theta) * sinf(phi));
      float z = center.z + (r * cosf(theta));

      samples->items[samples->count] = (Vector3){x, y, z};
      samples->count++;
    }
}

static void generate_data(const float cluster_radius, size_t k)
{
  SetRandomSeed(GetRandomValue(0, 10000));

  set.count = 0;

  Vector3 centers[k];

  float sphere_radius = cluster_radius * 2;

  generate_uniform_sphere_centers(sphere_radius, k, centers);

  float stddev = cluster_radius / 2; // Use cluster_radius as the standard deviation

  for(size_t i = 0; i < k; ++i) { cluster[i].count = 0; }
  for(size_t i = 0; i < k; ++i) { generate_cluster(centers[i], stddev, N, &set); }
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
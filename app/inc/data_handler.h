
#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H
#include "common.h"

static void generate_cluster(Vector3 center, float stddev, size_t count, Samples3D *samples);
void        generate_data(const float cluster_radius, size_t k);
void        generate_uniform_sphere_centers(float sphere_radius, size_t num_clusters, Vector3 *centers);
Vector3     sphericalToCartesian(float radius, float theta, float phi);
#endif
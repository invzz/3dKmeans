#ifndef KMEANS_H
#define KMEANS_H
#include "common.h"

void randomize_means(size_t k, float bound);
void reset_set(Samples3D *s);
void append_to_cluster(Samples3D *s, Vector3 p);
void recluster_state(size_t kl);
void update_means(float cluster_radius, size_t cluster_count, size_t k);

#endif

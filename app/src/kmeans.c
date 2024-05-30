
#include "kmeans.h"
#include "float.h"
#define C_ALPHA 0.2f
static void randomize_means(size_t k, float bound)
{
  for(size_t i = 0; i < k; ++i)
    {
      means[i].x        = Lerp(-bound, bound, (float)GetRandomValue(0, 100) / 100);
      means[i].y        = Lerp(-bound, bound, (float)GetRandomValue(0, 100) / 100);
      means[i].z        = Lerp(-bound, bound, (float)GetRandomValue(0, 100) / 100);
      target_means[i]   = means[i];
      old_means[i]      = means[i];
      cluster_colors[i] = ColorAlpha(colors[i % COLORS_COUNT], C_ALPHA);
      target_colors[i]  = ColorAlpha(cluster_colors[i], C_ALPHA);
      old_colors[i]     = cluster_colors[i];
    }
}

static void reset_set(Samples3D *s)
{
  if(s->items) free(s->items);
  s->items    = NULL;
  s->count    = 0;
  s->capacity = 0;
}

static void append_to_cluster(Samples3D *cluster, Vector3 p)
{
  if(cluster->capacity == 0)
    {
      cluster->capacity = 1;
      cluster->items    = malloc(cluster->capacity * sizeof(Vector3));
    }

  else if(cluster->count == cluster->capacity)
    {
      cluster->items = realloc(cluster->items, cluster->capacity * sizeof(Vector3) * 2);
      cluster->capacity *= 2;
    }
  cluster->items[cluster->count] = p;
  cluster->count++;
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
      if(k != -1) append_to_cluster(&cluster[k], p);
    }
  current_k = kl;
}

static void update_means(float cluster_radius, size_t k)
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
          target_means[i].x = Lerp(-cluster_radius * k, cluster_radius * k, (float)GetRandomValue(0, 100) / 100);
          target_means[i].y = Lerp(-cluster_radius * k, cluster_radius * k, (float)GetRandomValue(0, 100) / 100);
          target_means[i].z = Lerp(-cluster_radius * k, cluster_radius * k, (float)GetRandomValue(0, 100) / 100);
          target_colors[i]  = colors[i % COLORS_COUNT]; // Assign new color
        }
    }
  animation_time = 0.0f; // Reset animation time
}

import numpy as np
import sys


def generate_sphere_surface(radius, center, count):
  """Generates points uniformly distributed on the surface of a sphere."""
  # Generate random directions using normal distribution
  pts = np.random.normal(size=(count, 3))
  # Normalize vectors to lie exactly on the surface, then scale by radius
  norms = np.linalg.norm(pts, axis=1, keepdims=True)
  pts = (pts / norms) * radius
  return pts + np.array(center)


def generate_bounding_box_noise(bounds, count):
  """Generates uniform random noise in a bounding box."""
  return np.random.uniform(
      low=bounds[0], high=bounds[1], size=(count, 3)
  )


# --- Build the Dataset ---
np.random.seed(42)

# Two distinct spherical clusters + background noise
cluster1 = generate_sphere_surface(radius=2.0, center=[0, 0, 0], count=1000)
cluster2 = generate_sphere_surface(radius=1.0, center=[6, 0, 0], count=500)
noise = generate_bounding_box_noise(bounds=(-5.0, 8.0), count=300)

# Combine points and create ground-truth labels (-1 for noise, 0 and 1 for clusters)
points = np.vstack([cluster1, cluster2, noise])
labels = np.concatenate([
    np.zeros(len(cluster1), dtype=int),
    np.ones(len(cluster2), dtype=int),
    np.full(len(noise), -1, dtype=int),
])

# Save standard .xyz file (just coordinates)
np.savetxt(f"{sys.argv[1]}.xyz", points, fmt="%.6f", delimiter=" ")
# Save ground truth labels for your unit tests
np.savetxt(f"{sys.argv[1]}.lbl", labels, fmt="%d")

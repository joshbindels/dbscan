import numpy as np
from sklearn.cluster import DBSCAN
import sys


def run_dbscan_and_export(
    input_path, output_path, eps=0.5, min_samples=10
):
  # 1. Load the raw .xyz point cloud
  print(f"Loading points from {input_path}...")
  points = np.loadtxt(input_path)

  # 2. Run DBSCAN
  print(f"Running DBSCAN (eps={eps}, min_samples={min_samples})...")
  db = DBSCAN(eps=eps, min_samples=min_samples)
  labels = db.fit_predict(points)

  # Print quick summary stats
  n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
  n_noise = list(labels).count(-1)
  print(f"-> Detected {n_clusters} clusters.")
  print(f"-> Classified {n_noise} points as noise (-1).")

  # 3. Combine coordinates (x, y, z) with predicted labels
  output_data = np.column_stack([points, labels])

  # 4. Save as a 4-column text file
  np.savetxt(
      output_path,
      output_data,
      fmt=["%.6f", "%.6f", "%.6f", "%d"],
      delimiter=" ",
  )
  print(f"Saved labeled output to {output_path}")


run_dbscan_and_export(f"{sys.argv[1]}", f"{sys.argv[2]}.xyz", eps=float(sys.argv[3]), min_samples=int(sys.argv[4]))

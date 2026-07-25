#!/usr/bin/env python3
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# File paths
CSV_PATH = "merged_scan_results.csv"
OUTPUT_PLOT = "flux_distribution.png"

def main():
    if not os.path.exists(CSV_PATH):
        print(f"Error: {CSV_PATH} not found. Please run the merge script first.")
        return

    print("Loading merged simulation results...")
    df = pd.read_csv(CSV_PATH)

    # Display basic info
    print("\n--- Data Summary ---")
    print(df.info())
    print("\n--- Statistics ---")
    print(df.describe())

    # Create grid for plotting
    print("\nReshaping data for 2D imaging...")
    try:
        # Pivot x and y to create a 2D grid
        grid_flux = df.pivot(index='y', columns='x', values='normalizedFlux')
        grid_counts = df.pivot(index='y', columns='x', values='roiCounts')
        
        x_unique = np.sort(df['x'].unique())
        y_unique = np.sort(df['y'].unique())
        
        # Plotting
        fig, axes = plt.subplots(1, 2, figsize=(14, 6))

        # 1. Normalized Flux Heatmap
        im1 = axes[0].imshow(grid_flux, extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                             origin='lower', cmap='viridis', aspect='equal')
        axes[0].set_title("Normalized Flux Distribution")
        axes[0].set_xlabel("X (cm)")
        axes[0].set_ylabel("Y (cm)")
        axes[0].set_xlim(x_unique.min(), x_unique.max())
        axes[0].set_ylim(y_unique.min(), y_unique.max())
        fig.colorbar(im1, ax=axes[0], label="Normalized Flux")

        # 2. ROI Counts Heatmap
        im2 = axes[1].imshow(grid_counts, extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                             origin='lower', cmap='inferno', aspect='equal')
        axes[1].set_title("ROI Counts Distribution")
        axes[1].set_xlabel("X (cm)")
        axes[1].set_ylabel("Y (cm)")
        axes[1].set_xlim(x_unique.min(), x_unique.max())
        axes[1].set_ylim(y_unique.min(), y_unique.max())
        fig.colorbar(im2, ax=axes[1], label="ROI Counts")

        plt.tight_layout()
        plt.savefig(OUTPUT_PLOT, dpi=300)
        print(f"Successfully generated and saved plot to {OUTPUT_PLOT}")

        # Basic beam profiling analysis
        # Find peak position
        peak_idx = df['normalizedFlux'].idxmax()
        peak_row = df.loc[peak_idx]
        print(f"\n--- Beam Analysis ---")
        print(f"Peak Normalized Flux: {peak_row['normalizedFlux']:.6f}")
        print(f"Peak Location: X = {peak_row['x']} mm, Y = {peak_row['y']} mm")

        # Calculate beam center of gravity (centroid)
        total_flux = df['normalizedFlux'].sum()
        if total_flux > 0:
            x_cog = (df['x'] * df['normalizedFlux']).sum() / total_flux
            y_cog = (df['y'] * df['normalizedFlux']).sum() / total_flux
            print(f"Centroid (Center of Gravity): X = {x_cog:.3f} mm, Y = {y_cog:.3f} mm")

    except Exception as e:
        print(f"Error during pivot or plotting: {e}")
        print("Falling back to scatter plot...")
        
        # Fallback to scatter plot if data is not on a perfect rectangular grid
        plt.figure(figsize=(8, 6))
        sc = plt.scatter(df['x'], df['y'], c=df['normalizedFlux'], cmap='viridis', s=2)
        plt.colorbar(sc, label="Normalized Flux")
        plt.title("Normalized Flux Spatial Distribution (Scatter)")
        plt.xlabel("X (mm)")
        plt.ylabel("Y (mm)")
        plt.savefig(OUTPUT_PLOT, dpi=300)
        print(f"Successfully generated and saved scatter plot to {OUTPUT_PLOT}")

if __name__ == "__main__":
    main()

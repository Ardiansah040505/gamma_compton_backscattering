#!/usr/bin/env python3
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def main():
    csv_path = "merged_scan_results.csv"
    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found. Please run merge_scan_results.py first.")
        return

    print(f"Loading {csv_path}...")
    df = pd.read_csv(csv_path)

    # 1. Sum values of all 6 detectors at each (x, y) coordinate
    print("Summing values from all detectors at each (x, y) coordinate...")
    combined = df.groupby(['x', 'y'])[['roiCounts', 'normalizedCounts', 'flux', 'normalizedFlux']].sum().reset_index()

    # 2. Find the reference value (center at x=0, y=0 or the closest point to it)
    print("Finding center reference point (closest to x=0, y=0)...")
    # Calculate distance to (0,0)
    combined['dist_to_center'] = np.sqrt(combined['x']**2 + combined['y']**2)
    center_row = combined.loc[combined['dist_to_center'].idxmin()]
    ref_x, ref_y = center_row['x'], center_row['y']
    
    print(f"Reference Point Found: x = {ref_x}, y = {ref_y}")
    
    ref_values = {
        'roiCounts': center_row['roiCounts'],
        'normalizedFlux': center_row['normalizedFlux']
    }
    
    # Calculate overall grid mean as an alternative reference
    mean_values = {
        'roiCounts': combined['roiCounts'].mean(),
        'normalizedFlux': combined['normalizedFlux'].mean()
    }

    print(f"  * Reference values at center ({ref_x}, {ref_y}):")
    print(f"    - roiCounts: {ref_values['roiCounts']}")
    print(f"    - normalizedFlux: {ref_values['normalizedFlux']:.6f}")
    print(f"  * Mean values across entire grid:")
    print(f"    - roiCounts: {mean_values['roiCounts']:.2f}")
    print(f"    - normalizedFlux: {mean_values['normalizedFlux']:.6f}")

    # 3. Calculate Relative Difference
    # We will compute both: relative to center and relative to grid mean
    for col in ['roiCounts', 'normalizedFlux']:
        ref_val = ref_values[col]
        grid_mean = mean_values[col]
        
        # Relative to center
        if ref_val != 0:
            combined[f'{col}_rel_diff_to_center'] = (combined[col] - ref_val) / ref_val
        else:
            combined[f'{col}_rel_diff_to_center'] = 0.0
            
        # Relative to grid mean
        if grid_mean != 0:
            combined[f'{col}_rel_diff_to_mean'] = (combined[col] - grid_mean) / grid_mean
        else:
            combined[f'{col}_rel_diff_to_mean'] = 0.0

    # Save to CSV
    output_csv = "gcb_hor3_combined_relative_diff.csv"
    # Drop temp column before saving
    combined_save = combined.drop(columns=['dist_to_center'])
    combined_save.to_csv(output_csv, index=False)
    print(f"\nSaved combined relative differences to {output_csv}")

    # 4. Generate beautiful 2D heatmaps for normalizedFlux
    try:
        # Pivot the data for plotting
        grid_center_diff = combined.pivot(index='y', columns='x', values='normalizedFlux_rel_diff_to_center')
        grid_mean_diff = combined.pivot(index='y', columns='x', values='normalizedFlux_rel_diff_to_mean')
        
        x_unique = np.sort(combined['x'].unique())
        y_unique = np.sort(combined['y'].unique())

        fig, axes = plt.subplots(1, 2, figsize=(16, 7))

        # Heatmap 1: Relative to Center
        im1 = axes[0].imshow(grid_center_diff * 100, extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                             origin='lower', cmap='RdBu_r', aspect='equal')
        axes[0].set_title("Relative Diff of Normalized Flux (%) \n(Relative to Center [0,0])")
        axes[0].set_xlabel("X (cm)")
        axes[0].set_ylabel("Y (cm)")
        axes[0].scatter(ref_x, ref_y, color='black', marker='x', s=100, label='Center Ref')
        axes[0].legend()
        fig.colorbar(im1, ax=axes[0], label="Difference (%)")

        # Heatmap 2: Relative to Grid Mean
        im2 = axes[1].imshow(grid_mean_diff * 100, extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                             origin='lower', cmap='PRGn', aspect='equal')
        axes[1].set_title("Relative Diff of Normalized Flux (%) \n(Relative to Grid Mean)")
        axes[1].set_xlabel("X (cm)")
        axes[1].set_ylabel("Y (cm)")
        fig.colorbar(im2, ax=axes[1], label="Difference (%)")

        plt.suptitle("gcb_hor3: Combined Detectors Relative Difference Analysis", fontsize=16)
        plt.tight_layout()
        output_plot = "gcb_hor3_combined_relative_diff_map.png"
        plt.savefig(output_plot, dpi=300)
        print(f"Saved visualization map to {output_plot}")

    except Exception as e:
        print(f"Skipping plot creation due to error: {e}")

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
import os
import glob
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def main():
    # 1. Merge all CSV files
    print("Finding scan results CSV files in the current directory...")
    files = glob.glob("scan_results_*.csv")
    files = [f for f in files if "merged" not in f]

    def get_start_idx(filename):
        base = os.path.basename(filename)
        parts = base.split('_')
        if len(parts) >= 3:
            try:
                return int(parts[2])
            except ValueError:
                return 9999999
        return 9999999

    files.sort(key=get_start_idx)
    print(f"Found {len(files)} files to merge.")

    dfs = []
    for f in files:
        if os.path.getsize(f) == 0:
            print(f"Skipping empty file: {f}")
            continue
        try:
            df_temp = pd.read_csv(f)
            if df_temp.empty:
                print(f"Skipping empty dataframe in: {f}")
                continue
            dfs.append(df_temp)
        except Exception as e:
            print(f"Error reading {f}: {e}")

    if not dfs:
        print("No valid CSV files found to merge.")
        return

    merged_df = pd.concat(dfs, ignore_index=True)
    merged_csv_path = "merged_scan_results.csv"
    merged_df.to_csv(merged_csv_path, index=False)
    print(f"Successfully merged {len(dfs)} files into '{merged_csv_path}'")

    # 2. Pivot the data to get columns for each detector
    print("\nReshaping data to compute relative differences...")
    # Group by x, y and detectorID to handle any potential duplicate scans (take mean if duplicates exist)
    df_pivot = merged_df.pivot_table(
        index=['x', 'y'], 
        columns='detectorID', 
        values='normalizedCounts', 
        aggfunc='mean'
    ).reset_index()

    # Ensure all 6 detectors are present in the columns
    for i in range(6):
        if i not in df_pivot.columns:
            df_pivot[i] = 0.0

    # Rename detector columns for clarity
    df_pivot.rename(columns={i: f'det_{i}' for i in range(6)}, inplace=True)

    # 3. Calculate Relative Differences
    # Symmetric pairs across the horizontal axis:
    # Detector 1 (phi=60, top-right) vs Detector 4 (phi=240, bottom-left)
    # Detector 2 (phi=120, top-left) vs Detector 5 (phi=300, bottom-right)
    # Detector 0 (phi=0, middle-right) vs Detector 3 (phi=180, middle-left)

    # Formula for relative difference: (Det_top - Det_bottom) / ((Det_top + Det_bottom) / 2)
    # To avoid division by zero:
    epsilon = 1e-15
    
    # Relative difference between top and bottom detectors
    df_pivot['RD_1_4'] = (df_pivot['det_1'] - df_pivot['det_4']) / ((df_pivot['det_1'] + df_pivot['det_4']) / 2 + epsilon)
    df_pivot['RD_2_5'] = (df_pivot['det_2'] - df_pivot['det_5']) / ((df_pivot['det_2'] + df_pivot['det_5']) / 2 + epsilon)
    df_pivot['RD_0_3'] = (df_pivot['det_0'] - df_pivot['det_3']) / ((df_pivot['det_0'] + df_pivot['det_3']) / 2 + epsilon)

    # Save the detailed pivoted and analyzed data
    analyzed_csv_path = "analyzed_relative_difference.csv"
    df_pivot.to_csv(analyzed_csv_path, index=False)
    print(f"Saved analyzed relative differences to '{analyzed_csv_path}'")

    # Display statistics
    print("\n--- Relative Difference Statistics ---")
    print(df_pivot[['RD_1_4', 'RD_2_5', 'RD_0_3']].describe())

    # 4. Generate 2D Heatmaps
    print("\nGenerating 2D Heatmap plots...")
    try:
        x_unique = np.sort(df_pivot['x'].unique())
        y_unique = np.sort(df_pivot['y'].unique())
        
        # Grid shapes
        grid_shape = (len(y_unique), len(x_unique))
        
        # Pivot the relative differences back to 2D grids for plotting
        grid_RD_1_4 = df_pivot.pivot(index='y', columns='x', values='RD_1_4')
        grid_RD_2_5 = df_pivot.pivot(index='y', columns='x', values='RD_2_5')
        grid_RD_0_3 = df_pivot.pivot(index='y', columns='x', values='RD_0_3')

        fig, axes = plt.subplots(1, 3, figsize=(20, 6))

        extent = [x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()]

        # Plot RD_1_4
        im1 = axes[0].imshow(grid_RD_1_4, extent=extent, origin='lower', cmap='seismic', aspect='equal')
        axes[0].set_title("Relative Difference Det 1 vs 4\n(Top-Right vs Bottom-Left)")
        axes[0].set_xlabel("X (mm)")
        axes[0].set_ylabel("Y (mm)")
        fig.colorbar(im1, ax=axes[0], label="Relative Difference")

        # Plot RD_2_5
        im2 = axes[1].imshow(grid_RD_2_5, extent=extent, origin='lower', cmap='seismic', aspect='equal')
        axes[1].set_title("Relative Difference Det 2 vs 5\n(Top-Left vs Bottom-Right)")
        axes[1].set_xlabel("X (mm)")
        axes[1].set_ylabel("Y (mm)")
        fig.colorbar(im2, ax=axes[1], label="Relative Difference")

        # Plot RD_0_3
        im3 = axes[2].imshow(grid_RD_0_3, extent=extent, origin='lower', cmap='seismic', aspect='equal')
        axes[2].set_title("Relative Difference Det 0 vs 3\n(Middle-Right vs Middle-Left)")
        axes[2].set_xlabel("X (mm)")
        axes[2].set_ylabel("Y (mm)")
        fig.colorbar(im3, ax=axes[2], label="Relative Difference")

        plt.suptitle("Relative Difference Spatial Distributions (Defect detection)", fontsize=16)
        plt.tight_layout()
        
        plot_path = "relative_difference_plots.png"
        plt.savefig(plot_path, dpi=300)
        print(f"Successfully generated and saved plot to '{plot_path}'")

    except Exception as e:
        print(f"Error plotting 2D heatmaps: {e}")
        print("Falling back to scatter plot generation...")
        try:
            plt.figure(figsize=(10, 8))
            sc = plt.scatter(df_pivot['x'], df_pivot['y'], c=df_pivot['RD_1_4'], cmap='seismic', s=10)
            plt.colorbar(sc, label="Relative Difference Det 1 vs 4")
            plt.title("Relative Difference Det 1 vs 4 Spatial Distribution (Scatter)")
            plt.xlabel("X (mm)")
            plt.ylabel("Y (mm)")
            plt.savefig("relative_difference_scatter.png", dpi=300)
            print("Successfully saved scatter plot to 'relative_difference_scatter.png'")
        except Exception as sc_err:
            print(f"Failed to generate scatter plot: {sc_err}")

if __name__ == "__main__":
    main()

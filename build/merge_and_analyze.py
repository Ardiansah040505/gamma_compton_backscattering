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
    files = [f for f in files if "merged" not in f and "analyzed" not in f]

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

    # 2. Integrate all detectors by summing/averaging counts for each (x, y)
    print("\nIntegrating counts across all 6 detectors...")
    
    # Group by x and y and sum the counts
    df_integrated = merged_df.groupby(['x', 'y']).agg({
        'roiCounts': 'sum',
        'normalizedCounts': 'sum',
        'nPrimary': 'sum'
    }).reset_index()

    # Save the integrated data
    integrated_csv_path = "integrated_scan_results.csv"
    df_integrated.to_csv(integrated_csv_path, index=False)
    print(f"Saved integrated detector results to '{integrated_csv_path}'")

    # Display statistics
    print("\n--- Integrated Signal Statistics ---")
    print(df_integrated[['roiCounts', 'normalizedCounts']].describe())

    # 3. Generate 2D Heatmap of Integrated Image
    print("\nGenerating 2D integrated image...")
    try:
        x_unique = np.sort(df_integrated['x'].unique())
        y_unique = np.sort(df_integrated['y'].unique())
        
        # Pivot the integrated normalized counts to a 2D grid
        grid_integrated = df_integrated.pivot(index='y', columns='x', values='normalizedCounts')

        plt.figure(figsize=(10, 8))
        extent = [x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()]

        # Plot integrated image using a beautiful colormap (e.g., 'inferno' or 'viridis')
        im = plt.imshow(grid_integrated, extent=extent, origin='lower', cmap='inferno', aspect='equal')
        
        plt.title("Integrated 2D Compton Backscattering Image\n(Sum of All 6 Detectors)", fontsize=14, fontweight='bold', pad=15)
        plt.xlabel("X Position (cm)", fontsize=12)
        plt.ylabel("Y Position (cm)", fontsize=12)
        
        # Add colorbar
        cbar = plt.colorbar(im)
        cbar.set_label("Integrated Normalized Backscatter intensity (counts/primary)", fontsize=12)

        plt.tight_layout()
        
        plot_path = "integrated_backscatter_image.png"
        plt.savefig(plot_path, dpi=300)
        print(f"Successfully generated and saved integrated image to '{plot_path}'")

    except Exception as e:
        print(f"Error plotting 2D heatmap: {e}")
        print("Falling back to scatter plot generation...")
        try:
            plt.figure(figsize=(10, 8))
            sc = plt.scatter(df_integrated['x'], df_integrated['y'], c=df_integrated['normalizedCounts'], cmap='inferno', s=15)
            plt.colorbar(sc, label="Integrated Normalized Counts")
            plt.title("Integrated Spatial Distribution (Scatter)")
            plt.xlabel("X (cm)")
            plt.ylabel("Y (cm)")
            plt.savefig("integrated_backscatter_scatter.png", dpi=300)
            print("Successfully saved scatter plot to 'integrated_backscatter_scatter.png'")
        except Exception as sc_err:
            print(f"Failed to generate scatter plot: {sc_err}")

if __name__ == "__main__":
    main()

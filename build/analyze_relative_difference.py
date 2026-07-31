#!/usr/bin/env python3
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def main():
    # Today's 20 clean chunks
    chunks = [
        "0_124", "125_249", "250_374", "375_499", "500_624", "625_749", "750_874", "875_999",
        "1000_1124", "1125_1249", "1250_1374", "1375_1499", "1500_1624", "1625_1749", "1750_1874",
        "1875_1999", "2000_2124", "2125_2249", "2250_2374", "2375_2499"
    ]
    
    clean_files = [f"scan_results_{chunk}.csv" for chunk in chunks]
    
    print("Merging the 20 clean simulation files...")
    dfs = []
    for f in clean_files:
        if not os.path.exists(f):
            print(f"Warning: File {f} not found!")
            continue
        try:
            df_temp = pd.read_csv(f)
            dfs.append(df_temp)
        except Exception as e:
            print(f"Error reading {f}: {e}")

    if not dfs:
        print("No simulation files found to merge!")
        return

    merged_df = pd.concat(dfs, ignore_index=True)
    merged_df.to_csv("merged_scan_results.csv", index=False)
    print(f"Merged data saved to 'merged_scan_results.csv' ({len(merged_df)} rows).")

    # 1. Integrate Counts and Flux across all detectors
    print("\nIntegrating counts and fluxes across all detectors...")
    df_integrated = merged_df.groupby(['x', 'y']).agg({
        'roiCounts': 'sum',
        'normalizedCounts': 'sum',
        'flux': 'sum',
        'normalizedFlux': 'sum',
        'nPrimary': 'sum'
    }).reset_index()
    df_integrated.to_csv("integrated_scan_results.csv", index=False)
    print("Integrated results saved to 'integrated_scan_results.csv'.")

    # 2. Pivot data to analyze relative differences per detector pair
    # We want a dataframe with columns: x, y, c_det_0..c_det_5, f_det_0..f_det_5
    print("\nPreparing detector-specific tables...")
    pivot_counts = merged_df.pivot(index=['x', 'y'], columns='detectorID', values='normalizedCounts').reset_index()
    pivot_counts.columns = ['x', 'y'] + [f'det_{i}_counts' for i in range(6)]
    
    pivot_flux = merged_df.pivot(index=['x', 'y'], columns='detectorID', values='normalizedFlux').reset_index()
    pivot_flux.columns = ['x', 'y'] + [f'det_{i}_flux' for i in range(6)]
    
    pivot_df = pd.merge(pivot_counts, pivot_flux, on=['x', 'y'])

    # Relative difference formula: RD_A_B = 2 * (det_A - det_B) / (det_A + det_B)
    # Handling division by zero using np.where
    def calc_rd(det_A, det_B):
        denom = det_A + det_B
        # to avoid division by zero
        return np.where(denom > 0, 2.0 * (det_A - det_B) / denom, 0.0)

    print("Calculating relative differences for Counts and Flux...")
    # Counts relative differences
    pivot_df['RD_1_4_counts'] = calc_rd(pivot_df['det_1_counts'], pivot_df['det_4_counts'])
    pivot_df['RD_2_5_counts'] = calc_rd(pivot_df['det_2_counts'], pivot_df['det_5_counts'])
    pivot_df['RD_0_3_counts'] = calc_rd(pivot_df['det_0_counts'], pivot_df['det_3_counts'])

    # Flux relative differences
    pivot_df['RD_1_4_flux'] = calc_rd(pivot_df['det_1_flux'], pivot_df['det_4_flux'])
    pivot_df['RD_2_5_flux'] = calc_rd(pivot_df['det_2_flux'], pivot_df['det_5_flux'])
    pivot_df['RD_0_3_flux'] = calc_rd(pivot_df['det_0_flux'], pivot_df['det_3_flux'])

    # Save relative difference data
    pivot_df.to_csv("analyzed_relative_difference.csv", index=False)
    print("Relative differences saved to 'analyzed_relative_difference.csv'.")

    # Let's plot the 2D Heatmaps
    x_unique = np.sort(pivot_df['x'].unique())
    y_unique = np.sort(pivot_df['y'].unique())
    extent = [x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()]

    # Plot Function for a metric (Counts or Flux)
    def generate_rd_plot(metric_suffix, title_prefix, output_filename, cbar_label):
        fig, axes = plt.subplots(1, 3, figsize=(20, 5.5))
        pairs = [('1_4', 'RD 1-4'), ('2_5', 'RD 2-5'), ('0_3', 'RD 0-3')]
        
        for idx, (pair_name, pair_title) in enumerate(pairs):
            col_name = f'RD_{pair_name}_{metric_suffix}'
            # Pivot to 2D grid
            grid = pivot_df.pivot(index='y', columns='x', values=col_name)
            
            im = axes[idx].imshow(grid, extent=extent, origin='lower', cmap='seismic', aspect='equal', vmin=-2, vmax=2)
            axes[idx].set_title(f"{pair_title} ({title_prefix})", fontsize=12, fontweight='bold')
            axes[idx].set_xlabel("X Position (cm)")
            axes[idx].set_ylabel("Y Position (cm)")
            fig.colorbar(im, ax=axes[idx], label=cbar_label)
            
        plt.suptitle(f"2D Relative Difference Heatmaps - {title_prefix}", fontsize=16, fontweight='bold', y=0.98)
        plt.tight_layout()
        plt.savefig(output_filename, dpi=300)
        plt.close()
        print(f"Generated plot: {output_filename}")

    # Generate the plots
    generate_rd_plot('counts', 'Normalized Counts', 'relative_difference_plots.png', 'Relative Difference (Counts)')
    generate_rd_plot('flux', 'Normalized Flux', 'flux_relative_difference_plots.png', 'Relative Difference (Flux)')

    # Also update the integrated backscatter image
    print("\nUpdating integrated backscatter image...")
    try:
        grid_integrated = df_integrated.pivot(index='y', columns='x', values='normalizedCounts')
        plt.figure(figsize=(10, 8))
        im = plt.imshow(grid_integrated, extent=extent, origin='lower', cmap='inferno', aspect='equal')
        plt.title("Integrated 2D Compton Backscattering Image\n(Sum of All 6 Detectors - Counts)", fontsize=14, fontweight='bold', pad=15)
        plt.xlabel("X Position (cm)", fontsize=12)
        plt.ylabel("Y Position (cm)", fontsize=12)
        cbar = plt.colorbar(im)
        cbar.set_label("Integrated Normalized Counts", fontsize=12)
        plt.tight_layout()
        plt.savefig("integrated_backscatter_image.png", dpi=300)
        plt.close()
        print("Updated 'integrated_backscatter_image.png'.")
    except Exception as e:
        print(f"Error updating integrated backscatter image: {e}")

if __name__ == "__main__":
    main()

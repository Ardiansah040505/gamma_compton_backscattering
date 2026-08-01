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

    # 1. Calculate the mean values (across all detectors) for each coordinate (x, y)
    print("Calculating mean values across all detectors for each coordinate...")
    mean_df = df.groupby(['x', 'y'])[['roiCounts', 'normalizedCounts', 'flux', 'normalizedFlux']].mean().reset_index()
    mean_df.rename(columns={
        'roiCounts': 'mean_roiCounts',
        'normalizedCounts': 'mean_normalizedCounts',
        'flux': 'mean_flux',
        'normalizedFlux': 'mean_normalizedFlux'
    }, inplace=True)

    # 2. Merge mean values back into the original dataframe
    df_diff = pd.merge(df, mean_df, on=['x', 'y'])

    # 3. Calculate Relative Difference: (Value - Mean) / Mean
    # Using np.where to avoid division by zero
    for col in ['roiCounts', 'normalizedCounts', 'flux', 'normalizedFlux']:
        mean_col = f'mean_{col}'
        df_diff[f'{col}_rel_diff'] = np.where(
            df_diff[mean_col] != 0,
            (df_diff[col] - df_diff[mean_col]) / df_diff[mean_col],
            0.0
        )

    # 4. Save results to CSV
    output_csv = "gcb_hor3_relative_diff.csv"
    df_diff.to_csv(output_csv, index=False)
    print(f"Saved relative differences to {output_csv}")

    # 5. Print Summary per Detector
    print("\n" + "="*60)
    print(" RELATIVE DIFFERENCE SUMMARY PER DETECTOR (%) ".center(60, "="))
    print(" (Relative to the average of all detectors at each coordinate) ")
    print("="*60)
    
    for det_id in sorted(df_diff['detectorID'].unique()):
        det_data = df_diff[df_diff['detectorID'] == det_id]
        print(f"\nDetector ID: {det_id}")
        for col in ['roiCounts', 'normalizedFlux']:
            rel_diff_pct = det_data[f'{col}_rel_diff'] * 100
            print(f"  * {col:16s}: Mean Diff = {rel_diff_pct.mean():+.4f}%, StdDev = {rel_diff_pct.std():.4f}%, Range = [{rel_diff_pct.min():+.4f}%, {rel_diff_pct.max():+.4f}%]")
    print("="*60)

    # 6. Plot the spatial distribution of relative difference for each detector
    try:
        detectors = sorted(df_diff['detectorID'].unique())
        fig, axes = plt.subplots(2, 3, figsize=(18, 10))
        axes = axes.flatten()

        for idx, det_id in enumerate(detectors):
            det_data = df_diff[df_diff['detectorID'] == det_id]
            # Pivot for 2D imaging
            grid_diff = det_data.pivot(index='y', columns='x', values='normalizedFlux_rel_diff')
            x_unique = np.sort(det_data['x'].unique())
            y_unique = np.sort(det_data['y'].unique())

            im = axes[idx].imshow(grid_diff * 100, extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                                 origin='lower', cmap='RdBu_r', aspect='equal', vmin=-50, vmax=50)
            axes[idx].set_title(f"Detector {det_id} Relative Diff (%)")
            axes[idx].set_xlabel("X (cm)")
            axes[idx].set_ylabel("Y (cm)")
            fig.colorbar(im, ax=axes[idx], label="Relative Diff (%)")

        plt.suptitle("Relative Difference Map of normalizedFlux per Detector (%)", fontsize=16)
        plt.tight_layout()
        output_plot = "gcb_hor3_relative_diff_map.png"
        plt.savefig(output_plot, dpi=300)
        print(f"Saved plot to {output_plot}")

    except Exception as e:
        print(f"Skipping plot creation due to error: {e}")

if __name__ == "__main__":
    main()

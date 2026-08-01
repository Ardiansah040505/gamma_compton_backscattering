#!/usr/bin/env python3
import os
import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def calculate_differences(df1, df2, on_cols=['x', 'y', 'detectorID'], val_cols=['roiCounts', 'normalizedCounts', 'flux', 'normalizedFlux']):
    # Merge the two dataframes on matching coordinates and detectors
    merged = pd.merge(df1, df2, on=on_cols, suffixes=('_1', '_2'))
    
    if merged.empty:
        print("Warning: No matching rows found based on columns:", on_cols)
        return merged

    print(f"Aligned {len(merged)} data points for comparison.")
    
    # Calculate differences for each value column
    for col in val_cols:
        col1 = f"{col}_1"
        col2 = f"{col}_2"
        
        if col1 in merged.columns and col2 in merged.columns:
            # Absolute Difference: V2 - V1
            merged[f'{col}_abs_diff'] = merged[col2] - merged[col1]
            
            # Relative Difference: (V2 - V1) / V1
            # Using np.where to handle division by zero
            merged[f'{col}_rel_diff'] = np.where(
                merged[col1] != 0,
                (merged[col2] - merged[col1]) / merged[col1],
                np.nan
            )
            
            # Symmetric Relative Difference: (V2 - V1) / ((V1 + V2) / 2)
            denom = (merged[col1] + merged[col2]) / 2
            merged[f'{col}_sym_rel_diff'] = np.where(
                denom != 0,
                (merged[col2] - merged[col1]) / denom,
                np.nan
            )
            
    return merged

def print_summary(diff_df, val_cols=['roiCounts', 'normalizedCounts', 'flux', 'normalizedFlux']):
    print("\n" + "="*50)
    print(" DIFFERENCE ANALYSIS SUMMARY ".center(50, "="))
    print("="*50)
    
    for col in val_cols:
        rel_col = f'{col}_rel_diff'
        abs_col = f'{col}_abs_diff'
        
        if rel_col in diff_df.columns:
            # Drop NaN values for statistics
            rel_diffs = diff_df[rel_col].dropna()
            abs_diffs = diff_df[abs_col].dropna()
            
            if len(rel_diffs) == 0:
                continue
                
            mean_rel = rel_diffs.mean() * 100
            median_rel = rel_diffs.median() * 100
            max_rel = rel_diffs.max() * 100
            min_rel = rel_diffs.min() * 100
            std_rel = rel_diffs.std() * 100
            
            mean_abs = abs_diffs.mean()
            max_abs = abs_diffs.max()
            min_abs = abs_diffs.min()
            
            print(f"\nColumn: {col}")
            print(f"  * Absolute Diff: Mean = {mean_abs:.6f}, Range = [{min_abs:.6f}, {max_abs:.6f}]")
            print(f"  * Relative Diff: Mean = {mean_rel:.4f}%, Median = {median_rel:.4f}%, StdDev = {std_rel:.4f}%")
            print(f"  * Relative Diff Range: [{min_rel:.4f}%, {max_rel:.4f}%]")
    print("="*50)

def plot_differences(diff_df, output_path="relative_differences.png", val_col='normalizedFlux'):
    rel_col = f'{val_col}_rel_diff'
    if rel_col not in diff_df.columns:
        print(f"Cannot plot: Column {rel_col} not found in differences dataframe.")
        return
        
    df_plot = diff_df.dropna(subset=[rel_col])
    if df_plot.empty:
        print("No data available to plot.")
        return
        
    # Pivot for 2D spatial heatmap if possible (using average across detectors if multiple detectors exist per pixel)
    try:
        # Group by x, y to average differences across detectors
        grid_data = df_plot.groupby(['y', 'x'])[rel_col].mean().reset_index()
        grid_pivot = grid_data.pivot(index='y', columns='x', values=rel_col)
        
        x_unique = np.sort(grid_data['x'].unique())
        y_unique = np.sort(grid_data['y'].unique())
        
        fig, axes = plt.subplots(1, 2, figsize=(14, 6))
        
        # Heatmap of relative difference (converted to percentage)
        im = axes[0].imshow(grid_pivot * 100, extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                            origin='lower', cmap='RdBu_r', aspect='equal')
        axes[0].set_title(f"Relative Difference Map of {val_col} (%)")
        axes[0].set_xlabel("X")
        axes[0].set_ylabel("Y")
        fig.colorbar(im, ax=axes[0], label="Relative Difference (%)")
        
        # Histogram of relative differences
        axes[1].hist(df_plot[rel_col] * 100, bins=50, color='skyblue', edgecolor='black')
        axes[1].set_title(f"Distribution of Relative Differences for {val_col}")
        axes[1].set_xlabel("Relative Difference (%)")
        axes[1].set_ylabel("Frequency")
        axes[1].axvline(0, color='red', linestyle='--', linewidth=1)
        
        plt.tight_layout()
        plt.savefig(output_path, dpi=300)
        print(f"Saved relative difference plot to {output_path}")
        
    except Exception as e:
        print(f"Could not generate heatmap: {e}. Generating scatter plot and histogram instead.")
        plt.figure(figsize=(12, 5))
        
        plt.subplot(1, 2, 1)
        sc = plt.scatter(df_plot['x'], df_plot['y'], c=df_plot[rel_col]*100, cmap='RdBu_r', s=5)
        plt.colorbar(sc, label="Relative Difference (%)")
        plt.title(f"Relative Difference Scatter of {val_col} (%)")
        plt.xlabel("X")
        plt.ylabel("Y")
        
        plt.subplot(1, 2, 2)
        plt.hist(df_plot[rel_col]*100, bins=50, color='skyblue', edgecolor='black')
        plt.title(f"Distribution of Relative Differences for {val_col}")
        plt.xlabel("Relative Difference (%)")
        plt.ylabel("Frequency")
        plt.axvline(0, color='red', linestyle='--', linewidth=1)
        
        plt.tight_layout()
        plt.savefig(output_path, dpi=300)
        print(f"Saved relative difference scatter plot to {output_path}")

def main():
    parser = argparse.ArgumentParser(description="Compare two simulation results (CSV files) and calculate relative differences.")
    parser.add_argument("file1", help="Path to the first (baseline) CSV file")
    parser.add_argument("file2", help="Path to the second CSV file to compare against the first")
    parser.add_argument("--output", "-o", default="comparison_results.csv", help="Output path for the comparison CSV results")
    parser.add_argument("--plot", "-p", default="relative_differences.png", help="Output path for the generated plot image")
    parser.add_argument("--column", "-c", default="normalizedFlux", help="The primary value column to plot (default: normalizedFlux)")
    
    args = parser.parse_args()
    
    if not os.path.exists(args.file1):
        print(f"Error: Baseline file '{args.file1}' does not exist.")
        return
    if not os.path.exists(args.file2):
        print(f"Error: Comparison file '{args.file2}' does not exist.")
        return
        
    print(f"Loading {args.file1}...")
    df1 = pd.read_csv(args.file1)
    print(f"Loading {args.file2}...")
    df2 = pd.read_csv(args.file2)
    
    diff_df = calculate_differences(df1, df2)
    
    if not diff_df.empty:
        diff_df.to_csv(args.output, index=False)
        print(f"Saved comparison details to {args.output}")
        print_summary(diff_df)
        plot_differences(diff_df, args.plot, args.column)

if __name__ == "__main__":
    main()

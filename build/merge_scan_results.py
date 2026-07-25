#!/usr/bin/env python3
import os
import glob
import pandas as pd

def main():
    # Remove existing merged file first if it exists
    if os.path.exists("merged_scan_results.csv"):
        os.remove("merged_scan_results.csv")
        print("Deleted old merged_scan_results.csv")

    # Find all scan results csv files
    files = glob.glob("scan_results_*.csv")
    # Filter out merged file just in case
    files = [f for f in files if "merged" not in f]

    # Sort files by their starting index number
    def get_start_idx(filename):
        # e.g., scan_results_101972_109815.csv -> 101972
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

    # Read and merge
    dfs = []
    for f in files:
        print(f"Reading {f}...")
        dfs.append(pd.read_csv(f))

    if dfs:
        merged_df = pd.concat(dfs, ignore_index=True)
        merged_df.to_csv("merged_scan_results.csv", index=False)
        print("Successfully saved merged results to merged_scan_results.csv")
    else:
        print("No scan result CSV files found to merge.")

if __name__ == "__main__":
    main()

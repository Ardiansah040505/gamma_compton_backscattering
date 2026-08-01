#!/usr/bin/env python3
import glob
import os
import pandas as pd

def main():
    build_dir = "build"
    # Cari semua file scan_results_*.csv di folder build
    csv_files = glob.glob(os.path.join(build_dir, "scan_results_*.csv"))
    
    if not csv_files:
        print(f"Tidak ada file scan_results_*.csv di folder {build_dir}")
        return
        
    print(f"Menemukan {len(csv_files)} file CSV untuk digabungkan...")
    
    # Baca dan gabungkan semua file
    df_list = []
    for file in csv_files:
        df = pd.read_csv(file)
        df_list.append(df)
        
    merged_df = pd.concat(df_list, ignore_index=True)
    
    # Urutkan berdasarkan x, y, dan detectorID agar rapi
    merged_df.sort_values(by=['x', 'y', 'detectorID'], inplace=True)
    
    # Tentukan path output
    output_build = os.path.join(build_dir, "merged_scan_results.csv")
    output_crack = "crack_1/crack.csv"
    
    # Simpan ke build/merged_scan_results.csv
    merged_df.to_csv(output_build, index=False)
    print(f"Berhasil menggabungkan data ke: {output_build}")
    
    # Simpan/Copy ke crack_1/crack.csv
    os.makedirs(os.path.dirname(output_crack), exist_ok=True)
    merged_df.to_csv(output_crack, index=False)
    print(f"Berhasil memperbarui data retak (crack) di: {output_crack}")

if __name__ == "__main__":
    main()

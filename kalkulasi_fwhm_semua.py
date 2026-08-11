#!/usr/bin/env python3
import os
import glob
import pandas as pd
import numpy as np

def hitung_fwhm_text(csv_path):
    # 1. Load data
    try:
        df = pd.read_csv(csv_path)
    except Exception as e:
        return f"Error loading file: {e}"
        
    # Check required columns
    required_cols = {'x', 'y', 'roiCounts_crack'}
    if not required_cols.issubset(df.columns):
        return f"Missing columns. File has columns: {list(df.columns)}"

    # 2. Pivot data ke bentuk grid X, Y
    grid = df.pivot(index='x', columns='y', values='roiCounts_crack')
    
    # Koordinat sumbu X dan Y
    x_coords = grid.index.values
    y_coords = grid.columns.values
    
    # Profil retakan terjadi di sepanjang sumbu Y pada X=1 (retakan berada di sekitar Y=0)
    target_x = 1.0
    x_idx = np.abs(x_coords - target_x).argmin()
    
    # Ambil line profile sepanjang sumbu Y pada X=1
    profile = grid.iloc[x_idx, :].values # Cacah ROI (Counts)
    
    # Saring rentang Y di sekitar retakan (misal dari Y = -10 sampai 10) untuk kalkulasi FWHM
    mask = (y_coords >= -10) & (y_coords <= 10)
    y_slice = y_coords[mask]
    profile_slice = profile[mask]
    
    # 3. Hitung FWHM
    baseline = np.mean(profile_slice[np.abs(y_slice) > 5]) # Baseline di luar area retakan
    peak_val = np.min(profile_slice) # Lembah terdalam
    peak_idx = np.argmin(profile_slice)
    peak_y = y_slice[peak_idx]
    
    half_max = baseline + (peak_val - baseline) / 2.0
    
    # Interpolasi Linear
    left_y = y_slice[:peak_idx+1]
    left_prof = profile_slice[:peak_idx+1]
    
    right_y = y_slice[peak_idx:]
    right_prof = profile_slice[peak_idx:]
    
    # Sort for interpolation
    sort_left = np.argsort(left_prof)
    y1 = np.interp(half_max, left_prof[sort_left], left_y[sort_left])
    
    sort_right = np.argsort(right_prof)
    y2 = np.interp(half_max, right_prof[sort_right], right_y[sort_right])
    
    fwhm = abs(y2 - y1)
    
    return {
        "target_x": target_x,
        "peak_y": peak_y,
        "peak_val": peak_val,
        "baseline": baseline,
        "half_max": half_max,
        "y1": y1,
        "y2": y2,
        "fwhm": fwhm
    }

def main():
    dir_path = "/data/mahasiswa/ardian/gcb_project/gcb_hor/hasil_sementara"
    
    # List files of interest
    pattern = os.path.join(dir_path, "crack_normal_combined_relative_diff_*.csv")
    files = glob.glob(pattern)
    
    # Sort files by name for cleaner presentation
    files = sorted(files)
    
    print("=" * 80)
    print(" REKAPITULASI ANALISIS FWHM SEMUA RETAKAN ".center(80, "="))
    print("=" * 80)
    print(f"{'File / Varian':<45} | {'FWHM (cm)':<12} | {'Posisi Puncak':<15}")
    print("-" * 80)
    
    detailed_reports = []
    
    for f in files:
        filename = os.path.basename(f)
        res = hitung_fwhm_text(f)
        if isinstance(res, dict):
            print(f"{filename:<45} | {res['fwhm']:10.4f} cm | Y = {res['peak_y']:.2f} cm")
            detailed_reports.append((filename, res))
        else:
            # Maybe it's a file without roiCounts_crack, like a basic relative diff file
            pass
            
    print("=" * 80)
    print("\n")
    
    # Detailed report for each file
    for name, res in detailed_reports:
        print("=" * 60)
        print(f" DETAIL ANALISIS: {name} ".center(60, "-"))
        print(f"  Posisi Retakan Terdeteksi: X = {res['target_x']} cm, Y = {res['peak_y']} cm")
        print(f"  Nilai Drop Maksimum       : {res['peak_val']:.1f} counts")
        print(f"  Nilai Baseline            : {res['baseline']:.1f} counts")
        print(f"  Half Maximum (HM)         : {res['half_max']:.1f} counts")
        print(f"  Titik Potong HM (y1, y2)  : ({res['y1']:.4f} cm, {res['y2']:.4f} cm)")
        print(f"  Lebar FWHM Hasil Hitung   : {res['fwhm']:.4f} cm")
    print("=" * 60)

if __name__ == "__main__":
    main()

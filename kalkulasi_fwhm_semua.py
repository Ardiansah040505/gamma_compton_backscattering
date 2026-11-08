#!/usr/bin/env python3
"""
=============================================================================
KALKULASI FWHM DAN DETEKSI CACAT RETAKAN CIRCULAR (SEMUA UKURAN)
Proyek Deteksi Keretakan Pipa GCB (Gamma Compton Backscattering)
=============================================================================
"""

import os
import glob
import pandas as pd
import numpy as np

def calculate_fwhm_robust(coords, values, center=0.0, max_dist=2.5):
    coords = np.array(coords, dtype=float)
    values = np.array(values, dtype=float)
    
    mask_flank = np.abs(coords) >= 7.0
    if np.sum(mask_flank) >= 2:
        baseline = np.mean(values[mask_flank])
    else:
        baseline = (np.mean(values[:3]) + np.mean(values[-3:])) / 2.0
        
    p = baseline - values
    
    mask_c = np.abs(coords - center) <= max_dist
    if not np.any(mask_c):
        mask_c = np.ones_like(coords, dtype=bool)
    c_indices = np.where(mask_c)[0]
    p_max_idx = c_indices[np.argmax(p[mask_c])]
    p_max = p[p_max_idx]
    coord_peak = coords[p_max_idx]
    
    if p_max <= 0:
        return np.nan, np.nan, np.nan, coord_peak
        
    p_half = p_max / 2.0
    
    x_left = np.nan
    for i in range(p_max_idx - 1, -1, -1):
        if (p[i] <= p_half and p[i+1] >= p_half) or (p[i] >= p_half and p[i+1] <= p_half):
            x1, x2 = coords[i], coords[i+1]
            y1, y2 = p[i], p[i+1]
            if y2 != y1:
                x_left = x1 + (p_half - y1) * (x2 - x1) / (y2 - y1)
                break
                
    x_right = np.nan
    for i in range(p_max_idx, len(coords) - 1):
        if (p[i] >= p_half and p[i+1] <= p_half) or (p[i] <= p_half and p[i+1] >= p_half):
            x1, x2 = coords[i], coords[i+1]
            y1, y2 = p[i], p[i+1]
            if y2 != y1:
                x_right = x1 + (p_half - y1) * (x2 - x1) / (y2 - y1)
                break
                
    fwhm = x_right - x_left
    return fwhm, x_left, x_right, coord_peak

def parse_actual(filename):
    clean = os.path.basename(filename).replace("crack_normal_combined_relative_diff_", "").replace(".csv", "")
    if clean in ['crack1', '1cm', 'crack1_0cm', '1_0cm']:
        return 1.0
    if clean in ['crack2', '2cm', 'crack2_0cm', '2_0cm']:
        return 2.0
    clean = clean.replace('crack', '').replace('cm', '').replace('_', '.')
    try:
        return float(clean)
    except ValueError:
        return np.nan

def main():
    dir_path = os.path.dirname(os.path.abspath(__file__))
    pattern = os.path.join(dir_path, "crack_normal_combined_relative_diff_*.csv")
    files = sorted(glob.glob(pattern))
    
    if not files:
        print("Error: Tidak ada file data ditemukan.")
        return
        
    print("=" * 85)
    print(" REKAPITULASI ANALISIS FWHM KERETAKAN CIRCULAR ".center(85, "="))
    print("=" * 85)
    print(f"{'File / Varian':<48} | {'Aktual':<6} | {'FWHM (cm)':<10} | {'Puncak (cm)':<10}")
    print("-" * 85)
    
    rows = []
    for fpath in files:
        fname = os.path.basename(fpath)
        actual = parse_actual(fname)
        try:
            df = pd.read_csv(fpath)
            if not {'x', 'y', 'roiCounts_crack'}.issubset(df.columns):
                continue
            
            grid = df.pivot(index='x', columns='y', values='roiCounts_crack')
            x_coords = grid.index.values
            y_coords = grid.columns.values
            mask = (y_coords >= -10) & (y_coords <= 10)
            
            x0_idx = np.abs(x_coords - 0.0).argmin()
            prof = grid.iloc[x0_idx, :].values[mask]
            
            fwhm, l_val, r_val, pk_val = calculate_fwhm_robust(y_coords[mask], prof)
            print(f"{fname:<48} | {actual:6.2f} | {fwhm:10.4f} | Y = {pk_val:6.2f}")
            rows.append({
                'File': fname,
                'Actual_Width_cm': actual,
                'FWHM_roiCounts (cm)': fwhm,
                'Left_roi': l_val,
                'Right_roi': r_val,
                'Peak_roi': pk_val,
            })
        except Exception as e:
            continue
            
    print("=" * 85)
    
    # Simpan crack_fwhm_results.csv
    if rows:
        df_out = pd.DataFrame(rows).sort_values(by='Actual_Width_cm').reset_index(drop=True)
        out_csv = os.path.join(dir_path, "crack_fwhm_results.csv")
        df_out.to_csv(out_csv, index=False)
        print(f"\nHasil FWHM mentah disimpan ke: {out_csv}\n")

if __name__ == "__main__":
    main()

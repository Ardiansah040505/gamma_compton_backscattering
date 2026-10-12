#!/usr/bin/env python3
import os
import glob
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def calculate_fwhm(x, y_val):
    p = -y_val
    p_max_idx = np.argmax(p)
    p_max = p[p_max_idx]
    x_peak = x[p_max_idx]
    
    p_baseline = (p[0] + p[-1]) / 2.0
    delta_p = p_max - p_baseline
    if delta_p <= 0:
        return np.nan, np.nan, np.nan, np.nan
        
    p_half = p_baseline + delta_p / 2.0
    
    # Left side crossing
    x_left = np.nan
    for i in range(p_max_idx):
        if (p[i] <= p_half and p[i+1] >= p_half) or (p[i] >= p_half and p[i+1] <= p_half):
            x1, x2 = x[i], x[i+1]
            y1, y2 = p[i], p[i+1]
            if y2 != y1:
                x_left = x1 + (p_half - y1) * (x2 - x1) / (y2 - y1)
                break
                
    # Right side crossing
    x_right = np.nan
    for i in range(p_max_idx, len(x) - 1):
        if (p[i] >= p_half and p[i+1] <= p_half) or (p[i] <= p_half and p[i+1] >= p_half):
            x1, x2 = x[i], x[i+1]
            y1, y2 = p[i], p[i+1]
            if y2 != y1:
                x_right = x1 + (p_half - y1) * (x2 - x1) / (y2 - y1)
                break
                
    fwhm = x_right - x_left
    return fwhm, x_left, x_right, x_peak

def main():
    hasil_dir = "/data/mahasiswa/ardian/gcb_project/gcb_ver/hasil_sementara"
    pattern = os.path.join(hasil_dir, "crack_normal_combined_relative_diff_*.csv")
    files = sorted(glob.glob(pattern))
    
    if not files:
        print("Tidak ada file pencocokan yang ditemukan.")
        return
        
    results = []
    
    # Setup plotting for individual visualization
    plt.figure(figsize=(12, 8))
    
    for file_path in files:
        file_name = os.path.basename(file_path)
        parts = file_name.replace("crack_normal_combined_relative_diff_", "").replace(".csv", "")
        
        df = pd.read_csv(file_path)
        zoom = df[(df['x'] >= -5) & (df['x'] <= 5) & (df['y'] >= 15) & (df['y'] <= 30)]
        
        if zoom.empty:
            continue
            
        profile = zoom.groupby('x')['roiCounts_rel_diff'].mean().reset_index()
        x = profile['x'].values
        y_roi = profile['roiCounts_rel_diff'].values
        
        fwhm_roi, left_roi, right_roi, peak_roi = calculate_fwhm(x, y_roi)
        
        profile_flux = zoom.groupby('x')['normalizedFlux_rel_diff'].mean().reset_index()
        y_flux = profile_flux['normalizedFlux_rel_diff'].values
        fwhm_flux, left_flux, right_flux, peak_flux = calculate_fwhm(x, y_flux)
        
        results.append({
            "File": parts,
            "FWHM_roiCounts (cm)": fwhm_roi,
            "Peak_roi": peak_roi,
            "Left_roi": left_roi,
            "Right_roi": right_roi,
            "FWHM_Flux (cm)": fwhm_flux,
            "Peak_flux": peak_flux,
            "Left_flux": left_flux,
            "Right_flux": right_flux,
        })
        
        # Plot profile for roiCounts_rel_diff
        # Only plot a subset of files to avoid clutter
        if parts in ['crack1', 'crack1_5cm', 'crack2cm']:
            plt.plot(x, -y_roi * 100, '-o', label=f"{parts} (FWHM={fwhm_roi:.3f} cm)")
            # Plot the half maximum line for this profile
            p = -y_roi * 100
            p_max = np.max(p)
            p_baseline = (p[0] + p[-1]) / 2.0
            p_half = p_baseline + (p_max - p_baseline) / 2.0
            plt.hlines(p_half, left_roi, right_roi, colors='gray', linestyles='--')
            plt.plot([left_roi, right_roi], [p_half, p_half], 'x', color='red')
            
    plt.title("Profil Perbedaan Relatif roiCounts (%) dan Lebar FWHM", fontsize=14)
    plt.xlabel("Posisi X (cm)", fontsize=12)
    plt.ylabel("Penurunan roiCounts Relatif (%)", fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend(fontsize=11)
    
    plot_output_path = os.path.join(hasil_dir, "fwhm_visualization.png")
    plt.savefig(plot_output_path, dpi=300, bbox_inches='tight')
    plt.close()
    
    df_res = pd.DataFrame(results)
    print(df_res.to_string(index=False))
    
    df_res.to_csv(os.path.join(hasil_dir, "crack_fwhm_results.csv"), index=False)
    print(f"\nHasil perhitungan FWHM disimpan ke: {os.path.join(hasil_dir, 'crack_fwhm_results.csv')}")
    print(f"Plot visualisasi FWHM disimpan ke: {plot_output_path}")

if __name__ == "__main__":
    main()

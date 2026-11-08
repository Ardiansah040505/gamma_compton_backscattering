#!/usr/bin/env python3
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def main():
    csv_path = "crack_normal_combined_relative_diff_crack1_3cm.csv"
    output_img = "fwhm_1_3cm_result.png"
    
    # 1. Load data
    df = pd.read_csv(csv_path)
    
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
    
    # Interpolasi Linear Sederhana untuk mencari titik potong di kiri dan kanan puncak
    # Bagian kiri puncak (dari awal slice hingga peak_idx)
    left_y = y_slice[:peak_idx+1]
    left_prof = profile_slice[:peak_idx+1]
    
    # Bagian kanan puncak (dari peak_idx hingga akhir slice)
    right_y = y_slice[peak_idx:]
    right_prof = profile_slice[peak_idx:]
    
    # Cari interpolasi linear untuk mencari di mana profile memotong half_max
    # Karena np.interp membutuhkan koordinat x (dalam hal ini profil nilai) terurut naik,
    # kita harus mengurutkannya terlebih dahulu sebelum melakukan interpolasi.
    
    # Sisi Kiri Puncak (nilai profil turun dari baseline ke peak_val)
    sort_left = np.argsort(left_prof)
    y1 = np.interp(half_max, left_prof[sort_left], left_y[sort_left])
    
    # Sisi Kanan Puncak (nilai profil naik dari peak_val ke baseline)
    sort_right = np.argsort(right_prof)
    y2 = np.interp(half_max, right_prof[sort_right], right_y[sort_right])
    
    fwhm = abs(y2 - y1)
    
    print("="*50)
    print(" ANALISIS FWHM DATA KERETAKAN 1 CM ".center(50, "="))
    print(f"Posisi Retakan Terdeteksi: X = {target_x} cm, Y = {peak_y} cm")
    print(f"Nilai Drop Maksimum       : {peak_val:.1f} counts")
    print(f"Nilai Baseline            : {baseline:.1f} counts")
    print(f"Half Maximum (HM)         : {half_max:.1f} counts")
    print(f"Titik Potong HM (y1, y2)  : ({y1:.4f} cm, {y2:.4f} cm)")
    print(f"Lebar FWHM Hasil Hitung   : {fwhm:.4f} cm")
    print("="*50)
    
    # 4. Plot Visualisasi
    plt.figure(figsize=(10, 6))
    plt.plot(y_coords, profile, 'b.-', label='Profil Intensitas (Line Profile X = 1 cm)')
    plt.axhline(baseline, color='green', linestyle='--', label=f'Baseline ({baseline:.1f} counts)')
    plt.axhline(half_max, color='red', linestyle='--', label=f'Half Maximum ({half_max:.1f} counts)')
    
    plt.plot([y1, y2], [half_max, half_max], color='orange', marker='o', linewidth=2.5, 
             label=f'FWHM = {fwhm:.3f} cm (Rentang: {y1:.2f} s/d {y2:.2f} cm)')
    plt.fill_between(y_slice, profile_slice, baseline, where=(y_slice >= y1) & (y_slice <= y2), 
                     color='orange', alpha=0.2, label='Area FWHM')
                          
    plt.title("Analisis Lebar Retakan Menggunakan FWHM (Data 1 cm)", fontsize=13, fontweight='bold')
    plt.xlabel("Y (cm) - Posisi Melintang Retakan")
    plt.ylabel("Cacah ROI Detektor (Counts)")
    plt.xlim(-10, 10)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(loc='lower right')
    
    plt.savefig(output_img, dpi=300, bbox_inches='tight')
    print(f"Visualisasi FWHM berhasil disimpan ke: {output_img}")

if __name__ == "__main__":
    main()

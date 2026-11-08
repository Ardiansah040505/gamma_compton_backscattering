#!/usr/bin/env python3
"""
=============================================================================
GENERATOR GRAFIK ANALISIS ERROR KERETAKAN HORIZONTAL (NILAI ABSOLUT)
Proyek Deteksi Keretakan Pipa GCB (Gamma Compton Backscattering)
=============================================================================
Seluruh error dihitung dalam bentuk absolut (|error| >= 0) sehingga tidak ada
nilai negatif. Nilai 0 merepresentasikan kondisi ideal (zero error).
"""

import os
import glob
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def parse_actual_width(filename):
    """
    Ekstraksi ground truth (lebar retakan sebenarnya dalam cm) dari nama file.
    """
    clean = os.path.basename(filename).replace("crack_normal_combined_relative_diff_", "").replace(".csv", "")
    if clean in ['crack1', '1cm']:
        return 1.0
    if clean in ['crack2', '2cm', '2_0cm']:
        return 2.0
    if clean == 'crack0_9':
        return 0.9
    if clean == 'crack1_2cm':
        return 1.2
    
    clean = clean.replace('crack', '').replace('cm', '').replace('_', '.')
    try:
        return float(clean)
    except ValueError:
        return np.nan

def calculate_fwhm_robust(coords, values):
    """
    Menghitung FWHM secara robust pada profil lembah (intensitas turun di retakan).
    Menggunakan pencarian crossing half-maximum dari titik puncak ke luar (flank-crossing).
    """
    coords = np.array(coords, dtype=float)
    values = np.array(values, dtype=float)
    
    # Baseline dari tepi profil
    baseline = (np.mean(values[:3]) + np.mean(values[-3:])) / 2.0
    p = baseline - values  # Ubah lembah menjadi puncak positif
    
    p_max_idx = np.argmax(p)
    p_max = p[p_max_idx]
    coord_peak = coords[p_max_idx]
    
    if p_max <= 0:
        return np.nan, np.nan, np.nan, coord_peak
        
    p_half = p_max / 2.0
    
    # Sisi Kiri: Telusuri mundur dari puncak ke arah indeks 0
    x_left = np.nan
    for i in range(p_max_idx - 1, -1, -1):
        if (p[i] <= p_half and p[i+1] >= p_half) or (p[i] >= p_half and p[i+1] <= p_half):
            x1, x2 = coords[i], coords[i+1]
            y1, y2 = p[i], p[i+1]
            if y2 != y1:
                x_left = x1 + (p_half - y1) * (x2 - x1) / (y2 - y1)
                break
                
    # Sisi Kanan: Telusuri maju dari puncak ke arah akhir
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

def load_and_analyze_datasets(data_dir):
    """
    Menganalisis seluruh dataset keretakan horizontal yang valid.
    """
    # Dataset terverifikasi yang memiliki defek retakan aktif
    target_specs = [
        ('crack0.3cm', 'crack_normal_combined_relative_diff_0_3cm.csv', 0.3),
        ('crack0.5cm', 'crack_normal_combined_relative_diff_0_5cm.csv', 0.5),
        ('crack0.7cm', 'crack_normal_combined_relative_diff_0_7cm.csv', 0.7),
        ('crack0.8cm', 'crack_normal_combined_relative_diff_0_8cm.csv', 0.8),
        ('crack0.9cm', 'crack_normal_combined_relative_diff_0_9cm.csv', 0.9),
        ('crack1.0cm', 'crack_normal_combined_relative_diff_1cm.csv', 1.0),
        ('crack1.1cm', 'crack_normal_combined_relative_diff_1_1cm.csv', 1.1),
        ('crack1.2cm', 'crack_normal_combined_relative_diff_crack1_2cm.csv', 1.2),
        ('crack1.4cm', 'crack_normal_combined_relative_diff_1_4cm.csv', 1.4),
        ('crack1.6cm', 'crack_normal_combined_relative_diff_1_6cm.csv', 1.6),
        ('crack1.7cm', 'crack_normal_combined_relative_diff_1_7cm.csv', 1.7),
        ('crack1.9cm', 'crack_normal_combined_relative_diff_1_9cm.csv', 1.9),
        ('crack2.0cm', 'crack_normal_combined_relative_diff_2cm.csv', 2.0),
        ('crack2.1cm', 'crack_normal_combined_relative_diff_2_1cm.csv', 2.1),
        ('crack2.3cm', 'crack_normal_combined_relative_diff_2_3cm.csv', 2.3),
        ('crack2.5cm', 'crack_normal_combined_relative_diff_2_5cm.csv', 2.5),
    ]
    
    rows = []
    for tag, fname, actual_w in target_specs:
        fpath = os.path.join(data_dir, fname)
        if not os.path.exists(fpath):
            continue
            
        df = pd.read_csv(fpath)
        if not {'x', 'y', 'roiCounts_crack'}.issubset(df.columns):
            continue
            
        # 1. Metode Line Profile pada X = 1 cm (sesuai kalkulasi_fwhm_semua.py)
        grid_roi = df.pivot(index='x', columns='y', values='roiCounts_crack')
        x_coords = grid_roi.index.values
        y_coords = grid_roi.columns.values
        x_idx = np.abs(x_coords - 1.0).argmin()
        prof_roi_x1 = grid_roi.iloc[x_idx, :].values
        
        mask = (y_coords >= -10) & (y_coords <= 10)
        y_slice = y_coords[mask]
        
        fwhm_roi_x1, l_rx1, r_rx1, pk_rx1 = calculate_fwhm_robust(y_slice, prof_roi_x1[mask])
        
        # Flux pada X = 1 cm
        grid_flux = df.pivot(index='x', columns='y', values='normalizedFlux_crack')
        prof_flux_x1 = grid_flux.iloc[x_idx, :].values
        fwhm_flux_x1, l_fx1, r_fx1, pk_fx1 = calculate_fwhm_robust(y_slice, prof_flux_x1[mask])
        
        # 2. Metode Rata-Rata ROI X in [-5, 5] cm (metodologi spatial averaging)
        zoom = df[(df['x'] >= -5) & (df['x'] <= 5) & (df['y'] >= -10) & (df['y'] <= 10)]
        mean_roi = zoom.groupby('y')['roiCounts_crack'].mean().reset_index()
        mean_flux = zoom.groupby('y')['normalizedFlux_crack'].mean().reset_index()
        
        fwhm_roi_avg, _, _, _ = calculate_fwhm_robust(mean_roi['y'].values, mean_roi['roiCounts_crack'].values)
        fwhm_flux_avg, _, _, _ = calculate_fwhm_robust(mean_flux['y'].values, mean_flux['normalizedFlux_crack'].values)
        
        rows.append({
            'Tag': tag,
            'File': fname,
            'Actual_Width_cm': actual_w,
            # Hasil FWHM Line Profile X=1
            'FWHM_roiCounts (cm)': fwhm_roi_x1,
            'Left_roi': l_rx1,
            'Right_roi': r_rx1,
            'Peak_roi': pk_rx1,
            'FWHM_Flux (cm)': fwhm_flux_x1,
            'Left_flux': l_fx1,
            'Right_flux': r_fx1,
            'Peak_flux': pk_fx1,
            # Hasil FWHM Spatial Average X in [-5, 5]
            'FWHM_roi_AvgX_cm': fwhm_roi_avg,
            'FWHM_flux_AvgX_cm': fwhm_flux_avg,
        })
        
    df_res = pd.DataFrame(rows).sort_values(by='Actual_Width_cm').reset_index(drop=True)
    
    # Kalkulasi Error Bertanda (untuk evaluasi) dan Error Nilai Mutlak (Absolut)
    # Error = FWHM - Actual
    df_res['Error_roi (cm)'] = df_res['FWHM_roiCounts (cm)'] - df_res['Actual_Width_cm']
    df_res['Rel_Error_roi (%)'] = (df_res['Error_roi (cm)'] / df_res['Actual_Width_cm']) * 100.0
    df_res['Error_flux (cm)'] = df_res['FWHM_Flux (cm)'] - df_res['Actual_Width_cm']
    df_res['Rel_Error_flux (%)'] = (df_res['Error_flux (cm)'] / df_res['Actual_Width_cm']) * 100.0
    
    # NILAI ABSOLUT SESUAI PERMINTAAN USER: |Error| >= 0
    df_res['Abs_Error_roi_cm'] = np.abs(df_res['Error_roi (cm)'])
    df_res['Abs_Rel_Error_roi_pct'] = np.abs(df_res['Rel_Error_roi (%)'])
    df_res['Abs_Error_flux_cm'] = np.abs(df_res['Error_flux (cm)'])
    df_res['Abs_Rel_Error_flux_pct'] = np.abs(df_res['Rel_Error_flux (%)'])
    
    # Rata-rata spasial absolut
    df_res['Abs_Error_roi_AvgX_cm'] = np.abs(df_res['FWHM_roi_AvgX_cm'] - df_res['Actual_Width_cm'])
    df_res['Abs_Rel_Error_roi_AvgX_pct'] = (df_res['Abs_Error_roi_AvgX_cm'] / df_res['Actual_Width_cm']) * 100.0
    df_res['Abs_Error_flux_AvgX_cm'] = np.abs(df_res['FWHM_flux_AvgX_cm'] - df_res['Actual_Width_cm'])
    df_res['Abs_Rel_Error_flux_AvgX_pct'] = (df_res['Abs_Error_flux_AvgX_cm'] / df_res['Actual_Width_cm']) * 100.0
    
    # Model Kalibrasi Polinomial Derajat 2: d_true = a*W^2 + b*W + c
    fwhm_arr = df_res['FWHM_roiCounts (cm)'].values
    true_arr = df_res['Actual_Width_cm'].values
    coeffs = np.polyfit(fwhm_arr, true_arr, 2)
    calibrated = np.polyval(coeffs, fwhm_arr)
    df_res['Calibrated_roi_cm'] = calibrated
    df_res['Abs_Error_Calibrated_cm'] = np.abs(calibrated - true_arr)
    df_res['Abs_Rel_Error_Calibrated_pct'] = (df_res['Abs_Error_Calibrated_cm'] / true_arr) * 100.0
    
    return df_res, coeffs

def plot_dual_panel(df, out_path):
    """
    Plot 1: Dual-Panel Subplots (Error Absolut dalam cm dan Error Relatif Absolut dalam %).
    """
    x = df['Actual_Width_cm']
    abs_err_roi = df['Abs_Error_roi_cm']
    abs_err_flx = df['Abs_Error_flux_cm']
    rel_err_roi = df['Abs_Rel_Error_roi_pct']
    rel_err_flx = df['Abs_Rel_Error_flux_pct']
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6), dpi=300)
    
    # Panel (a): Error Absolut (cm)
    ax1.axhline(0, color='black', linestyle='--', linewidth=1.2, alpha=0.8, label='Ideal / Zero Error (0 cm)')
    ax1.axvline(1.0, color='gray', linestyle=':', linewidth=1.0, alpha=0.7, label='Batas Resolusi Kolimator (~1.0 cm)')
    
    ax1.plot(x, abs_err_roi, 'o-', color='#1f77b4', linewidth=2.2, markersize=6, 
             label=r'Error Absolut roiCounts ($|\Delta W_{roi}|$)')
    ax1.plot(x, abs_err_flx, 's--', color='#d62728', linewidth=1.8, markersize=5, 
             label=r'Error Absolut normalizedFlux ($|\Delta W_{flux}|$)')
             
    ax1.set_title("(a) Error Absolut FWHM vs Lebar Keretakan Aktual", fontsize=12, fontweight='bold', pad=12)
    ax1.set_xlabel("Lebar Keretakan Sebenarnya / Target (cm)", fontsize=11, fontweight='bold')
    ax1.set_ylabel(r"Error Absolut: $|W_{FWHM} - W_{aktual}|$ (cm)", fontsize=11, fontweight='bold')
    ax1.set_xlim(0.1, 2.7)
    ax1.set_ylim(0.0, 0.85)
    ax1.grid(True, linestyle=':', alpha=0.6)
    ax1.legend(loc='upper right', fontsize=9.5, framealpha=0.9)
    
    # Panel (b): Error Relatif Absolut (%)
    ax2.axhline(0, color='black', linestyle='--', linewidth=1.2, alpha=0.8, label='Ideal / Zero Error (0%)')
    ax2.axvline(1.0, color='gray', linestyle=':', linewidth=1.0, alpha=0.7)
    
    ax2.plot(x, rel_err_roi, 'o-', color='#1f77b4', linewidth=2.2, markersize=6, 
             label='Error Relatif Absolut (%) roiCounts')
    ax2.plot(x, rel_err_flx, 's--', color='#d62728', linewidth=1.8, markersize=5, 
             label='Error Relatif Absolut (%) normalizedFlux')
             
    # Anotasi titik 0.3 cm
    ax2.annotate(f"0.3 cm: {rel_err_roi.iloc[0]:.1f}%", 
                 xy=(x.iloc[0], rel_err_roi.iloc[0]), 
                 xytext=(x.iloc[0]+0.2, rel_err_roi.iloc[0]-25),
                 arrowprops=dict(arrowstyle="->", color='#1f77b4', lw=1.2),
                 fontsize=9, fontweight='bold', color='#1f77b4')
                 
    ax2.set_title("(b) Error Relatif Absolut (%) vs Lebar Keretakan Aktual", fontsize=12, fontweight='bold', pad=12)
    ax2.set_xlabel("Lebar Keretakan Sebenarnya / Target (cm)", fontsize=11, fontweight='bold')
    ax2.set_ylabel(r"Error Relatif Absolut: $\frac{|W_{FWHM} - W_{aktual}|}{W_{aktual}} \times 100\%$", 
                   fontsize=11, fontweight='bold')
    ax2.set_xlim(0.1, 2.7)
    ax2.set_ylim(0.0, 260)
    ax2.grid(True, linestyle=':', alpha=0.6)
    ax2.legend(loc='upper right', fontsize=9.5, framealpha=0.9)
    
    fig.suptitle("Analisis Error Mutlak Pengukuran Keretakan Horizontal GCB (0.3 s/d 2.5 cm)", 
                 fontsize=14, fontweight='bold', y=0.98)
    plt.tight_layout(rect=[0, 0.02, 1, 0.95])
    fig.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close(fig)

def plot_dual_axis(df, out_path):
    """
    Plot 2: Single Canvas Twin Y-Axis (Sumbu Kiri = cm absolut, Sumbu Kanan = % absolut).
    """
    x = df['Actual_Width_cm']
    abs_err_roi = df['Abs_Error_roi_cm']
    rel_err_roi = df['Abs_Rel_Error_roi_pct']
    
    fig, ax_left = plt.subplots(figsize=(11, 6), dpi=300)
    ax_right = ax_left.twinx()
    
    # Sumbu Kiri: Error Absolut (cm)
    line1 = ax_left.plot(x, abs_err_roi, 'o-', color='#1f77b4', linewidth=2.5, markersize=7, 
                         label='Error Absolut |ΔW| (cm) - Sumbu Kiri')
    ax_left.axhline(0, color='gray', linestyle='--', linewidth=1.2, alpha=0.7)
    ax_left.set_xlabel("Lebar Keretakan Sebenarnya (cm)", fontsize=11, fontweight='bold')
    ax_left.set_ylabel("Error Absolut |ΔW| (cm)", color='#1f77b4', fontsize=11, fontweight='bold')
    ax_left.tick_params(axis='y', labelcolor='#1f77b4')
    ax_left.set_ylim(0.0, 0.85)
    ax_left.set_xlim(0.1, 2.7)
    ax_left.grid(True, linestyle=':', alpha=0.5)
    
    # Sumbu Kanan: Error Relatif Absolut (%)
    line2 = ax_right.plot(x, rel_err_roi, 's--', color='#e65100', linewidth=2.0, markersize=6, 
                          label='Error Relatif Absolut (%) - Sumbu Kanan')
    ax_right.set_ylabel("Error Relatif Absolut (%)", color='#e65100', fontsize=11, fontweight='bold')
    ax_right.tick_params(axis='y', labelcolor='#e65100')
    ax_right.set_ylim(0.0, 260)
    
    # Zona dominasi blur PSF kolimator
    ax_left.axvspan(0.1, 1.0, color='#ffebee', alpha=0.5, label='Zona Dominasi Blur Kolimator/PSF (< 1.0 cm)')
    
    lines = line1 + line2
    labels = [l.get_label() for l in lines]
    ax_left.legend(lines, labels, loc='upper center', bbox_to_anchor=(0.5, 0.95), fontsize=10, framealpha=0.9)
    
    plt.title("Grafik Terpadu (Dual Y-Axis): Error Absolut (cm) & Relatif Absolut (%) Keretakan Horizontal", 
              fontsize=13, fontweight='bold', pad=15)
    plt.tight_layout()
    fig.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close(fig)

def plot_fwhm_vs_actual(df, out_path):
    """
    Plot 3: Perbandingan FWHM terukur vs Lebar Aktual (roiCounts vs normalizedFlux vs Ideal line).
    """
    fig, ax = plt.subplots(figsize=(10, 6), dpi=300)
    
    x = df['Actual_Width_cm']
    y_roi = df['FWHM_roiCounts (cm)']
    y_flux = df['FWHM_Flux (cm)']
    
    ax.plot([0, 2.8], [0, 2.8], 'k--', linewidth=1.5, alpha=0.7, label='Ideal (FWHM = Lebar Sebenarnya)')
    ax.plot(x, y_roi, 'o-', color='#1f77b4', linewidth=2.2, markersize=6, label='FWHM Terukur (roiCounts)')
    ax.plot(x, y_flux, 's--', color='#d62728', linewidth=1.8, markersize=5, label='FWHM Terukur (normalizedFlux)')
    
    for i, txt in enumerate(x):
        ax.annotate(f"{txt:.1f}", (x.iloc[i], y_roi.iloc[i]), textcoords="offset points", 
                    xytext=(0, 6), ha='center', fontsize=8, color='#1f77b4')
                    
    ax.set_title("Perbandingan Lebar Keretakan Horizontal Terukur FWHM vs Lebar Sebenarnya", 
                 fontsize=13, fontweight='bold', pad=12)
    ax.set_xlabel("Lebar Keretakan Sebenarnya / Ground Truth (cm)", fontsize=11, fontweight='bold')
    ax.set_ylabel("Lebar Keretakan Hasil Pengukuran FWHM (cm)", fontsize=11, fontweight='bold')
    ax.set_xlim(0.0, 2.8)
    ax.set_ylim(0.0, 2.8)
    ax.grid(True, linestyle=':', alpha=0.6)
    ax.legend(loc='upper left', fontsize=10, framealpha=0.9)
    
    plt.tight_layout()
    fig.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close(fig)

def plot_calibration_reduction(df, coeffs, out_path):
    """
    Plot 4: Efektivitas Penurunan Error Menggunakan Kalibrasi Polinomial.
    """
    fig, ax = plt.subplots(figsize=(10, 6), dpi=300)
    
    x = df['Actual_Width_cm']
    raw_err = df['Abs_Error_roi_cm']
    cal_err = df['Abs_Error_Calibrated_cm']
    
    mae_raw = np.mean(raw_err)
    mae_cal = np.mean(cal_err)
    
    ax.axhline(0, color='black', linestyle='--', linewidth=1.2, alpha=0.8, label='Ideal / Zero Error (0 cm)')
    ax.plot(x, raw_err, 'o-', color='#d32f2f', linewidth=2.0, markersize=6, 
            label=f"Error FWHM Mentah (MAE = {mae_raw:.4f} cm)")
    ax.plot(x, cal_err, 's-', color='#1976d2', linewidth=2.2, markersize=6, 
            label=f"Setelah Kalibrasi Polinomial (MAE = {mae_cal:.4f} cm)")
            
    # Area arsir reduksi error
    ax.fill_between(x, cal_err, raw_err, color='#e3f2fd', alpha=0.6, label='Area Reduksi Error')
    
    eq_str = f"d = ({coeffs[0]:.4f})W² + ({coeffs[1]:.4f})W + ({coeffs[2]:.4f})"
    ax.text(0.05, 0.88, f"Model Kalibrasi:\n{eq_str}", transform=ax.transAxes, 
            fontsize=9.5, bbox=dict(boxstyle='round', facecolor='white', edgecolor='#1976d2', alpha=0.9))
            
    ax.set_title("Penurunan Error Absolut Keretakan Horizontal dengan Kalibrasi Polinomial", 
                 fontsize=13, fontweight='bold', pad=12)
    ax.set_xlabel("Lebar Keretakan Sebenarnya (cm)", fontsize=11, fontweight='bold')
    ax.set_ylabel("Error Absolut |ΔW| (cm)", fontsize=11, fontweight='bold')
    ax.set_xlim(0.1, 2.7)
    ax.set_ylim(0.0, 0.85)
    ax.grid(True, linestyle=':', alpha=0.6)
    ax.legend(loc='upper right', fontsize=10, framealpha=0.9)
    
    plt.tight_layout()
    fig.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close(fig)

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)
    
    print("=" * 85)
    print(" ANALISIS ERROR KERETAKAN HORIZONTAL GCB (NILAI ABSOLUT) ".center(85, "="))
    print("=" * 85)
    print(f"Direktori Data   : {parent_dir}")
    print(f"Direktori Output : {script_dir}")
    print("=" * 85)
    
    # 1. Analisis Data
    df, coeffs = load_and_analyze_datasets(parent_dir)
    if df.empty:
        print("Error: Tidak ada data keretakan yang berhasil dianalisis.")
        return
        
    print(f"\nBerhasil menganalisis {len(df)} variasi ukuran keretakan horizontal (0.3 s/d 2.5 cm).\n")
    
    # 2. Cetak Rekapitulasi ke Terminal
    print("-" * 85)
    print(f"{'Tag':<12} | {'Aktual':<6} | {'FWHM (cm)':<10} | {'|Err| (cm)':<10} | {'|%Err| (%)':<10} | {'Kalibrasi':<10} | {'|Err Kal|':<10}")
    print("-" * 85)
    for _, row in df.iterrows():
        print(f"{row['Tag']:<12} | {row['Actual_Width_cm']:6.2f} | {row['FWHM_roiCounts (cm)']:10.4f} | "
              f"{row['Abs_Error_roi_cm']:10.4f} | {row['Abs_Rel_Error_roi_pct']:9.2f}% | "
              f"{row['Calibrated_roi_cm']:10.4f} | {row['Abs_Error_Calibrated_cm']:10.4f}")
    print("-" * 85)
    
    mae_raw = df['Abs_Error_roi_cm'].mean()
    mae_cal = df['Abs_Error_Calibrated_cm'].mean()
    reduction = ((mae_raw - mae_cal) / mae_raw) * 100.0
    print(f"\nMean Absolute Error (FWHM Mentah) : {mae_raw:.4f} cm ({mae_raw*10:.2f} mm)")
    print(f"Mean Absolute Error (Kalibrasi)   : {mae_cal:.4f} cm ({mae_cal*10:.2f} mm)")
    print(f"Persentase Penurunan Error         : {reduction:.2f}%\n")
    
    # 3. Simpan CSV Rekapitulasi
    rekap_csv = os.path.join(script_dir, "rekap_error_keseluruhan.csv")
    df.to_csv(rekap_csv, index=False)
    print(f"[CSV Rekap] Disimpan ke: {rekap_csv}")
    
    # Simpan juga fwhm_vs_actual_summary.csv di parent directory (standar proyek)
    parent_summary_csv = os.path.join(parent_dir, "fwhm_vs_actual_summary.csv")
    df.to_csv(parent_summary_csv, index=False)
    print(f"[CSV Parent] Disimpan ke: {parent_summary_csv}")
    
    # 4. Generate Grafik
    # (a) Dual Panel
    out_dual_panel = os.path.join(script_dir, "grafik_error_dual_panel.png")
    plot_dual_panel(df, out_dual_panel)
    print(f"[Grafik 1/4] Disimpan: {out_dual_panel}")
    
    # (b) Dual Axis
    out_dual_axis = os.path.join(script_dir, "grafik_error_dual_axis.png")
    plot_dual_axis(df, out_dual_axis)
    print(f"[Grafik 2/4] Disimpan: {out_dual_axis}")
    
    # (c) FWHM vs Actual Comparison
    out_fwhm_comp = os.path.join(script_dir, "grafik_komparasi_fwhm_vs_aktual.png")
    plot_fwhm_vs_actual(df, out_fwhm_comp)
    print(f"[Grafik 3/4] Disimpan: {out_fwhm_comp}")
    
    # (d) Calibration Reduction
    out_cal_red = os.path.join(script_dir, "grafik_reduksi_error_kalibrasi.png")
    plot_calibration_reduction(df, coeffs, out_cal_red)
    print(f"[Grafik 4/4] Disimpan: {out_cal_red}")
    
    print("\nSeluruh grafik error nilai absolut dan rekapitulasi data berhasil digenerate!\n")

if __name__ == '__main__':
    main()

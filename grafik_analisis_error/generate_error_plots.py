#!/usr/bin/env python3
"""
=============================================================================
GENERATOR GRAFIK ANALISIS ERROR GABUNGAN (NILAI ABSOLUT)
Proyek Deteksi Keretakan Pipa GCB (Gamma Compton Backscattering)
=============================================================================
Seluruh error dihitung dalam bentuk absolut (|error| >= 0) sehingga tidak ada
nilai negatif. Nilai 0 merepresentasikan kondisi ideal (zero error).
"""

import os
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(base_dir)
    
    # Path file input
    summary_path = os.path.join(parent_dir, "fwhm_vs_actual_summary.csv")
    wiener_path = os.path.join(parent_dir, "deconvolution_fwhm_comparison.csv")
    rl_path = os.path.join(parent_dir, "richardson_lucy_fwhm_comparison.csv")
    
    if not os.path.exists(summary_path):
        print(f"Error: {summary_path} tidak ditemukan.")
        return
        
    df_summary = pd.read_csv(summary_path).sort_values(by='Actual_Width_cm').reset_index(drop=True)
    df_wiener = pd.read_csv(wiener_path).sort_values(by='Actual_cm').reset_index(drop=True) if os.path.exists(wiener_path) else None
    df_rl = pd.read_csv(rl_path).sort_values(by='Actual_cm').reset_index(drop=True) if os.path.exists(rl_path) else None

    # Styling umum matplotlib
    plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
    plt.rcParams['axes.edgecolor'] = '#333333'
    plt.rcParams['axes.linewidth'] = 1.0

    x = df_summary['Actual_Width_cm']
    
    # ERROR DALAM NILAI ABSOLUT (|error| >= 0)
    abs_err_roi = np.abs(df_summary['Error_roi (cm)'])
    abs_rel_err_roi = np.abs(df_summary['Rel_Error_roi (%)'])
    abs_err_flux = np.abs(df_summary['Error_flux (cm)'])
    abs_rel_err_flux = np.abs(df_summary['Rel_Error_flux (%)'])

    # =========================================================================
    # 1. GRAFIK DUAL-PANEL (SUBPLOT 1x2): Error Absolut (cm) & Relatif Absolut (%)
    # =========================================================================
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6), dpi=300)

    # Panel Kiri: Error Absolut |W_fwhm - W_aktual|
    ax1.axhline(0, color='black', linestyle='--', linewidth=1.2, alpha=0.8, label='Ideal / Zero Error (0 cm)')
    ax1.axvline(1.0, color='gray', linestyle=':', linewidth=1.0, alpha=0.7, label='Batas Resolusi Sinar (~1.0 cm)')

    ax1.plot(x, abs_err_roi, 'o-', color='#1f77b4', linewidth=2.2, markersize=6, 
             label=r'Error Absolut roiCounts ($|\Delta W_{roi}|$)')
    ax1.plot(x, abs_err_flux, 's--', color='#d62728', linewidth=1.8, markersize=5, 
             label=r'Error Absolut Flux ($|\Delta W_{flux}|$)')

    ax1.set_title("(a) Error Absolut FWHM vs Lebar Aktual", fontsize=12, fontweight='bold', pad=12)
    ax1.set_xlabel("Lebar Keretakan Sebenarnya (cm)", fontsize=11, fontweight='bold')
    ax1.set_ylabel(r"Error Absolut: $|W_{FWHM} - W_{aktual}|$ (cm)", fontsize=11, fontweight='bold')
    ax1.set_xlim(0.0, 2.6)
    ax1.set_ylim(0.0, 0.75)
    ax1.grid(True, linestyle=':', alpha=0.6)
    ax1.legend(loc='upper right', fontsize=9.5, framealpha=0.9)

    # Panel Kanan: Error Relatif Absolut (%)
    ax2.axhline(0, color='black', linestyle='--', linewidth=1.2, alpha=0.8, label='Ideal / Zero Error (0%)')
    ax2.axvline(1.0, color='gray', linestyle=':', linewidth=1.0, alpha=0.7)

    ax2.plot(x, abs_rel_err_roi, 'o-', color='#1f77b4', linewidth=2.2, markersize=6, 
             label='Error Relatif Absolut (%) roiCounts')
    ax2.plot(x, abs_rel_err_flux, 's--', color='#d62728', linewidth=1.8, markersize=5, 
             label='Error Relatif Absolut (%) Flux')

    # Anotasi titik ekstrem retakan mikro 0.1 cm
    ax2.annotate(f"0.1 cm: {abs_rel_err_roi.iloc[0]:.1f}%", 
                 xy=(x.iloc[0], abs_rel_err_roi.iloc[0]), 
                 xytext=(x.iloc[0]+0.2, abs_rel_err_roi.iloc[0]-35),
                 arrowprops=dict(arrowstyle="->", color='#1f77b4', lw=1.2),
                 fontsize=9, fontweight='bold', color='#1f77b4')

    ax2.set_title("(b) Error Relatif Absolut (%) vs Lebar Aktual", fontsize=12, fontweight='bold', pad=12)
    ax2.set_xlabel("Lebar Keretakan Sebenarnya (cm)", fontsize=11, fontweight='bold')
    ax2.set_ylabel(r"Error Relatif Absolut: $\frac{|W_{FWHM} - W_{aktual}|}{W_{aktual}} \times 100\%$", 
                   fontsize=11, fontweight='bold')
    ax2.set_xlim(0.0, 2.6)
    ax2.set_ylim(0.0, 600)
    ax2.grid(True, linestyle=':', alpha=0.6)
    ax2.legend(loc='upper right', fontsize=9.5, framealpha=0.9)

    fig.suptitle("Analisis Error Mutlak Pengukuran Keretakan Pipa GCB di Seluruh Ukuran (0.1 - 2.5 cm)", 
                 fontsize=14, fontweight='bold', y=0.98)
    plt.tight_layout(rect=[0, 0.02, 1, 0.95])
    
    out1 = os.path.join(base_dir, "grafik_error_dual_panel.png")
    fig.savefig(out1, dpi=300, bbox_inches='tight')
    plt.close(fig)
    print(f"[1/4] Disimpan: {out1}")

    # =========================================================================
    # 2. GRAFIK DUAL-Y AXIS (TWIN AXIS): 1 Kanvas Terintegrasi (Absolut cm & %)
    # =========================================================================
    fig, ax_left = plt.subplots(figsize=(11, 6), dpi=300)
    ax_right = ax_left.twinx()

    # Sumbu Kiri: Error Absolut (cm)
    line1 = ax_left.plot(x, abs_err_roi, 'o-', color='#1f77b4', linewidth=2.5, markersize=7, 
                         label='Error Absolut |ΔW| (cm) - Sumbu Kiri')
    ax_left.axhline(0, color='gray', linestyle='--', linewidth=1.2, alpha=0.7)
    ax_left.set_xlabel("Lebar Keretakan Sebenarnya (cm)", fontsize=11, fontweight='bold')
    ax_left.set_ylabel("Error Absolut |ΔW| (cm)", color='#1f77b4', fontsize=11, fontweight='bold')
    ax_left.tick_params(axis='y', labelcolor='#1f77b4')
    ax_left.set_ylim(0.0, 0.75)
    ax_left.set_xlim(0.0, 2.6)
    ax_left.grid(True, linestyle=':', alpha=0.5)

    # Sumbu Kanan: Error Relatif Absolut (%)
    line2 = ax_right.plot(x, abs_rel_err_roi, 's--', color='#e65100', linewidth=2.0, markersize=6, 
                          label='Error Relatif Absolut (%) - Sumbu Kanan')
    ax_right.set_ylabel("Error Relatif Absolut (%)", color='#e65100', fontsize=11, fontweight='bold')
    ax_right.tick_params(axis='y', labelcolor='#e65100')
    ax_right.set_ylim(0.0, 600)

    # Area bayangan zona blur PSF
    ax_left.axvspan(0.0, 1.0, color='#ffebee', alpha=0.5, label='Zona Dominasi Blur Kolimator/PSF (<1.0 cm)')

    # Gabung legend
    lines = line1 + line2
    labels = [l.get_label() for l in lines]
    ax_left.legend(lines, labels, loc='upper center', bbox_to_anchor=(0.5, 0.95), fontsize=10, framealpha=0.9)

    plt.title("Grafik Tunggal Terpadu (Dual Y-Axis): Error Absolut (cm) & Relatif Absolut (%)", 
              fontsize=13, fontweight='bold', pad=15)
    plt.tight_layout()

    out2 = os.path.join(base_dir, "grafik_error_dual_axis.png")
    fig.savefig(out2, dpi=300, bbox_inches='tight')
    plt.close(fig)
    print(f"[2/4] Disimpan: {out2}")

    # =========================================================================
    # 3. GRAFIK KOMPARASI METODE DEKONVOLUSI: Nilai Absolut
    # =========================================================================
    if df_wiener is not None and df_rl is not None:
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6), dpi=300)

        # Panel A: Error Absolut Antar-Metode
        ax1.axhline(0, color='black', linestyle='--', linewidth=1.2, alpha=0.7)
        ax1.plot(df_wiener['Actual_cm'], np.abs(df_wiener['Err_Raw_cm']), 'o-', color='#757575', 
                 linewidth=2.0, markersize=5, label='FWHM Mentah (Raw)')
        ax1.plot(df_wiener['Actual_cm'], np.abs(df_wiener['Err_Dec_cm']), 's-', color='#1e88e5', 
                 linewidth=2.2, markersize=6, label='Dekonvolusi Wiener')
        ax1.plot(df_rl['Actual_cm'], np.abs(df_rl['Err_RL_cm']), '^--', color='#43a047', 
                 linewidth=2.2, markersize=6, label='Dekonvolusi Richardson-Lucy')

        ax1.set_title("(a) Perbandingan Error Absolut Antar-Metode (cm)", fontsize=12, fontweight='bold')
        ax1.set_xlabel("Lebar Keretakan Sebenarnya (cm)", fontsize=11, fontweight='bold')
        ax1.set_ylabel(r"Error Absolut: $|W_{terukur} - W_{aktual}|$ (cm)", fontsize=11, fontweight='bold')
        ax1.set_xlim(0.0, 2.6)
        ax1.set_ylim(0.0, 1.05)
        ax1.grid(True, linestyle=':', alpha=0.6)
        ax1.legend(loc='upper right', fontsize=9.5)

        # Panel B: Error Relatif Absolut Antar-Metode
        ax2.axhline(0, color='black', linestyle='--', linewidth=1.2, alpha=0.7)
        ax2.plot(df_wiener['Actual_cm'], np.abs(df_wiener['RelErr_Raw_pct']), 'o-', color='#757575', 
                 linewidth=2.0, markersize=5, label='FWHM Mentah (Raw)')
        ax2.plot(df_wiener['Actual_cm'], np.abs(df_wiener['RelErr_Dec_pct']), 's-', color='#1e88e5', 
                 linewidth=2.2, markersize=6, label='Dekonvolusi Wiener')
        ax2.plot(df_rl['Actual_cm'], np.abs(df_rl['RelErr_RL_pct']), '^--', color='#43a047', 
                 linewidth=2.2, markersize=6, label='Dekonvolusi Richardson-Lucy')

        ax2.set_title("(b) Perbandingan Error Relatif Absolut Antar-Metode (%)", fontsize=12, fontweight='bold')
        ax2.set_xlabel("Lebar Keretakan Sebenarnya (cm)", fontsize=11, fontweight='bold')
        ax2.set_ylabel("Error Relatif Absolut (%)", fontsize=11, fontweight='bold')
        ax2.set_xlim(0.0, 2.6)
        ax2.set_ylim(0.0, 600)
        ax2.grid(True, linestyle=':', alpha=0.6)
        ax2.legend(loc='upper right', fontsize=9.5)

        fig.suptitle("Efektivitas Dekonvolusi (Wiener vs Richardson-Lucy) dalam Mereduksi Error Absolut", 
                     fontsize=14, fontweight='bold', y=0.98)
        plt.tight_layout(rect=[0, 0.02, 1, 0.95])

        out3 = os.path.join(base_dir, "grafik_komparasi_metode_dekonvolusi.png")
        fig.savefig(out3, dpi=300, bbox_inches='tight')
        plt.close(fig)
        print(f"[3/4] Disimpan: {out3}")

    # =========================================================================
    # 4. GRAFIK REDUKSI ERROR DENGAN KALIBRASI POLINOMIAL (Absolut)
    # =========================================================================
    if df_wiener is not None and 'Calibrated_Raw_cm' in df_wiener.columns:
        fig, ax = plt.subplots(figsize=(10, 6), dpi=300)
        
        abs_raw_err = np.abs(df_wiener['Err_Raw_cm'])
        abs_cal_raw = np.abs(df_wiener['Calibrated_Raw_cm'] - df_wiener['Actual_cm'])
        abs_cal_dec = np.abs(df_wiener['Calibrated_Dec_cm'] - df_wiener['Actual_cm'])

        ax.axhline(0, color='black', linestyle='--', linewidth=1.2, alpha=0.8, label='Ideal / Zero Error (0 cm)')
        ax.plot(df_wiener['Actual_cm'], abs_raw_err, 'o-', color='#d32f2f', 
                linewidth=2.0, markersize=6, label=f"Error FWHM Mentah (MAE = {np.mean(abs_raw_err):.3f} cm)")
        ax.plot(df_wiener['Actual_cm'], abs_cal_raw, 's-', color='#1976d2', 
                linewidth=2.2, markersize=6, label=f"Setelah Kalibrasi Polinomial (MAE = {np.mean(abs_cal_raw):.3f} cm)")
        ax.plot(df_wiener['Actual_cm'], abs_cal_dec, '^-', color='#388e3c', 
                linewidth=2.2, markersize=6, label=f"Setelah Dekonvolusi + Kalibrasi (MAE = {np.mean(abs_cal_dec):.3f} cm)")

        ax.set_title("Penurunan Error Absolut Pengukuran Menggunakan Model Kalibrasi Polinomial", 
                     fontsize=13, fontweight='bold', pad=12)
        ax.set_xlabel("Lebar Keretakan Sebenarnya (cm)", fontsize=11, fontweight='bold')
        ax.set_ylabel("Error Absolut |ΔW| (cm)", fontsize=11, fontweight='bold')
        ax.set_xlim(0.0, 2.6)
        ax.set_ylim(0.0, 0.7)
        ax.grid(True, linestyle=':', alpha=0.6)
        ax.legend(loc='upper right', fontsize=10, framealpha=0.9)

        plt.tight_layout()
        out4 = os.path.join(base_dir, "grafik_reduksi_error_kalibrasi.png")
        fig.savefig(out4, dpi=300, bbox_inches='tight')
        plt.close(fig)
        print(f"[4/4] Disimpan: {out4}")

    # =========================================================================
    # 5. REKAPITULASI TABEL ERROR GABUNGAN (Semua Nilai Absolut) KE CSV
    # =========================================================================
    rekap_df = pd.DataFrame({
        'Tag': df_summary['File'],
        'Lebar_Aktual_cm': df_summary['Actual_Width_cm'],
        'FWHM_roiCounts_cm': df_summary['FWHM_roiCounts (cm)'],
        'Abs_Error_roi_cm': abs_err_roi,
        'Abs_Rel_Error_roi_pct': abs_rel_err_roi,
        'FWHM_Flux_cm': df_summary['FWHM_Flux (cm)'],
        'Abs_Error_flux_cm': abs_err_flux,
        'Abs_Rel_Error_flux_pct': abs_rel_err_flux
    })
    
    if df_wiener is not None:
        rekap_df['FWHM_Wiener_cm'] = df_wiener['FWHM_Dec_cm']
        rekap_df['Abs_Error_Wiener_cm'] = np.abs(df_wiener['Err_Dec_cm'])
        rekap_df['Abs_Rel_Error_Wiener_pct'] = np.abs(df_wiener['RelErr_Dec_pct'])
        rekap_df['Hasil_Kalibrasi_cm'] = df_wiener['Calibrated_Raw_cm']
        rekap_df['Abs_Error_Kalibrasi_cm'] = np.abs(df_wiener['Calibrated_Raw_cm'] - df_wiener['Actual_cm'])

    out_csv = os.path.join(base_dir, "rekap_error_keseluruhan.csv")
    rekap_df.to_csv(out_csv, index=False)
    print(f"[Tabel] Disimpan: {out_csv}")

    # Eksekusi makro ROOT C untuk standarisasi grafik ROOT
    print("\n[INFO] Menjalankan makro ROOT C: generate_error_plots.C ...")
    macro_path = os.path.join(base_dir, "generate_error_plots.C")
    os.system(f"root -l -b -q {macro_path}")

    print("\nSeluruh grafik nilai absolut dan rekapitulasi error selesai diproses!")

if __name__ == "__main__":
    main()

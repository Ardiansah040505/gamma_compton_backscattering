#!/usr/bin/env python3
import os
import glob
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def parse_actual(filename):
    if filename == 'crack1':
        return 1.0
    if filename == 'crack2cm':
        return 2.0
    s = filename.replace('crack', '').replace('cm', '').replace('_', '.')
    return float(s)

def main():
    hasil_dir = "/data/mahasiswa/ardian/gcb_project/gcb_ver/hasil_sementara"
    fwhm_csv = os.path.join(hasil_dir, "crack_fwhm_results.csv")
    
    if not os.path.exists(fwhm_csv):
        print(f"Error: {fwhm_csv} tidak ditemukan.")
        return
        
    df = pd.read_csv(fwhm_csv)
    df['Actual_Width_cm'] = df['File'].apply(parse_actual)
    df = df.sort_values(by='Actual_Width_cm').reset_index(drop=True)
    
    # Calculate errors
    df['Error_roi (cm)'] = df['FWHM_roiCounts (cm)'] - df['Actual_Width_cm']
    df['Rel_Error_roi (%)'] = (df['Error_roi (cm)'] / df['Actual_Width_cm']) * 100
    
    df['Error_flux (cm)'] = df['FWHM_Flux (cm)'] - df['Actual_Width_cm']
    df['Rel_Error_flux (%)'] = (df['Error_flux (cm)'] / df['Actual_Width_cm']) * 100
    
    # Save detailed analysis CSV
    output_summary_csv = os.path.join(hasil_dir, "fwhm_vs_actual_summary.csv")
    df.to_csv(output_summary_csv, index=False)
    print(f"Hasil analisis FWHM detail disimpan ke: {output_summary_csv}")
    
    # 1. Chart: FWHM vs Actual Crack Width
    plt.figure(figsize=(10, 6))
    
    x = df['Actual_Width_cm']
    y_roi = df['FWHM_roiCounts (cm)']
    y_flux = df['FWHM_Flux (cm)']
    
    plt.plot([0, 2.7], [0, 2.7], 'k--', label='Ideal (FWHM = Lebar Sebenarnya)', linewidth=1.5, alpha=0.7)
    plt.plot(x, y_roi, 'o-', color='navy', linewidth=2, markersize=7, label='FWHM (roiCounts)')
    plt.plot(x, y_flux, 's--', color='crimson', linewidth=2, markersize=6, label='FWHM (normalizedFlux)')
    
    for i, txt in enumerate(df['Actual_Width_cm']):
        plt.annotate(f"{txt:.1f}", (x[i], y_roi[i]), textcoords="offset points", xytext=(0,6), ha='center', fontsize=8)
        
    plt.title("Perbandingan Lebar Retakan Terukur FWHM vs Lebar Retakan Sebenarnya", fontsize=13, fontweight='bold')
    plt.xlabel("Lebar Retakan Sebenarnya / Target (cm)", fontsize=11)
    plt.ylabel("Lebar Retakan Hasil FWHM (cm)", fontsize=11)
    plt.xlim(0, 2.7)
    plt.ylim(0, 2.7)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(loc='upper left', fontsize=10)
    
    plot_path = os.path.join(hasil_dir, "fwhm_vs_actual_comparison_chart.png")
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')
    plt.close()
    
    # 2. Chart: Errors Comparison (Absolute & Relative Error for roiCounts vs Flux)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
    
    # Left subplot: Absolute Error (cm)
    ax1.plot(x, df['Error_roi (cm)'], 'o-', color='navy', linewidth=2, label='Error Absolute (roiCounts)')
    ax1.plot(x, df['Error_flux (cm)'], 's--', color='crimson', linewidth=2, label='Error Absolute (normalizedFlux)')
    ax1.axhline(0, color='black', linestyle=':', alpha=0.8)
    ax1.set_title("Perbedaan Absolut FWHM (FWHM - Actual)", fontsize=12, fontweight='bold')
    ax1.set_xlabel("Lebar Retakan Sebenarnya (cm)", fontsize=11)
    ax1.set_ylabel("Error Absolut (cm)", fontsize=11)
    ax1.grid(True, linestyle=':', alpha=0.6)
    ax1.legend(fontsize=10)
    
    # Right subplot: Relative Error (%)
    ax2.plot(x, df['Rel_Error_roi (%)'], 'o-', color='navy', linewidth=2, label='Error Relatif (roiCounts)')
    ax2.plot(x, df['Rel_Error_flux (%)'], 's--', color='crimson', linewidth=2, label='Error Relatif (normalizedFlux)')
    ax2.axhline(0, color='black', linestyle=':', alpha=0.8)
    ax2.set_title("Perbedaan Relatif FWHM (%)", fontsize=12, fontweight='bold')
    ax2.set_xlabel("Lebar Retakan Sebenarnya (cm)", fontsize=11)
    ax2.set_ylabel("Error Relatif (%)", fontsize=11)
    ax2.grid(True, linestyle=':', alpha=0.6)
    ax2.legend(fontsize=10)
    
    plt.suptitle("Analisis Perbandingan Error (roiCounts vs normalizedFlux)", fontsize=14, y=0.98)
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    error_plot_path = os.path.join(hasil_dir, "fwhm_error_comparison_chart.png")
    plt.savefig(error_plot_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Grafik perbandingan error (roiCounts vs flux) disimpan ke: {error_plot_path}")

if __name__ == "__main__":
    main()

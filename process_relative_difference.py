#!/usr/bin/env python3
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def main():
    # Definisikan path input
    normal_path = "normal/normal.csv"
    crack_path = "crack_1/crack.csv"
    
    # Cek keberadaan file
    if not os.path.exists(normal_path):
        print(f"Error: File '{normal_path}' tidak ditemukan.")
        return
    if not os.path.exists(crack_path):
        print(f"Error: File '{crack_path}' tidak ditemukan.")
        return
        
    print(f"Membaca data normal dari: {normal_path}...")
    df_normal = pd.read_csv(normal_path)
    
    print(f"Membaca data retak (crack) dari: {crack_path}...")
    df_crack = pd.read_csv(crack_path)
    
    # 1. Gabungkan (jumlahkan) nilai dari semua detektor untuk setiap koordinat (x, y)
    print("Menjumlahkan nilai dari semua detektor untuk setiap koordinat (x, y)...")
    val_cols = ['roiCounts', 'normalizedCounts', 'flux', 'normalizedFlux']
    
    df_normal_combined = df_normal.groupby(['x', 'y'])[val_cols].sum().reset_index()
    df_crack_combined = df_crack.groupby(['x', 'y'])[val_cols].sum().reset_index()
    
    # 2. Gabungkan DataFrame data crack dan normal yang sudah dijumlahkan
    merged = pd.merge(df_crack_combined, df_normal_combined, on=['x', 'y'], suffixes=('_crack', '_normal'))
    
    if merged.empty:
        print("Error: Tidak ada data yang cocok setelah penggabungan koordinat.")
        return
        
    print(f"Berhasil menyelaraskan {len(merged)} koordinat (x, y) untuk perbandingan.")
    
    # 3. Hitung relative difference untuk nilai gabungan
    for col in val_cols:
        col_crack = f"{col}_crack"
        col_normal = f"{col}_normal"
        
        # Perbedaan Absolut (Crack - Normal)
        merged[f'{col}_abs_diff'] = merged[col_crack] - merged[col_normal]
        
        # Perbedaan Relatif: (Crack - Normal) / Normal
        merged[f'{col}_rel_diff'] = np.where(
            merged[col_normal] != 0,
            (merged[col_crack] - merged[col_normal]) / merged[col_normal],
            0.0
        )
        
        # Perbedaan Relatif Simetris: (Crack - Normal) / Rata-rata keduanya
        denom = (merged[col_crack] + merged[col_normal]) / 2
        merged[f'{col}_sym_rel_diff'] = np.where(
            denom != 0,
            (merged[col_crack] - merged[col_normal]) / denom,
            0.0
        )
        
    # Simpan hasil ke CSV
    output_csv = "crack_normal_combined_relative_diff.csv"
    merged.to_csv(output_csv, index=False)
    print(f"Hasil perhitungan gabungan berhasil disimpan ke: {output_csv}")
    
    # Tampilkan ringkasan statistik
    print("\n" + "="*70)
    print(" RINGKASAN PERBANDINGAN GABUNGAN DETEKTOR (ALL DETECTORS MERGED) ".center(70, "="))
    print(" Formula: (Crack_Total - Normal_Total) / Normal_Total ")
    print("="*70)
    
    for col in val_cols:
        rel_col = f'{col}_rel_diff'
        abs_col = f'{col}_abs_diff'
        
        rel_diffs = merged[rel_col] * 100  # ke persen
        abs_diffs = merged[abs_col]
        
        print(f"\nKolom: {col}")
        print(f"  * Perbedaan Absolut : Rata-rata = {abs_diffs.mean():+.6f}, Rentang = [{abs_diffs.min():+.6f}, {abs_diffs.max():+.6f}]")
        print(f"  * Perbedaan Relatif : Rata-rata = {rel_diffs.mean():+.4f}%, Standar Deviasi = {rel_diffs.std():.4f}%")
        print(f"  * Rentang Persen    : [{rel_diffs.min():+.4f}%, {rel_diffs.max():+.4f}%]")
    print("="*70)
    
    # 4. Buat peta heatmap spasial 2D untuk nilai gabungan
    try:
        # Pilih target plot, contoh: 'roiCounts_rel_diff' dan 'normalizedFlux_rel_diff'
        fig, axes = plt.subplots(1, 2, figsize=(16, 7))
        
        targets = ['roiCounts_rel_diff', 'normalizedFlux_rel_diff']
        titles = ['Relative Difference of Combined roiCounts (%)', 'Relative Difference of Combined normalizedFlux (%)']
        
        x_unique = np.sort(merged['x'].unique())
        y_unique = np.sort(merged['y'].unique())
        
        for idx, target_col in enumerate(targets):
            grid_diff = merged.pivot(index='y', columns='x', values=target_col)
            
            im = axes[idx].imshow(
                grid_diff * 100, 
                extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                origin='lower', 
                cmap='RdBu_r', 
                aspect='equal'
            )
            axes[idx].set_title(titles[idx], fontsize=12)
            axes[idx].set_xlabel("X (cm)")
            axes[idx].set_ylabel("Y (cm)")
            fig.colorbar(im, ax=axes[idx], label="Difference (%)")
            
        plt.suptitle("Peta Perbedaan Relatif Gabungan Detektor (Crack vs Normal)", fontsize=14, y=0.98)
        plt.tight_layout()
        
        output_plot = "crack_normal_combined_relative_diff_map.png"
        plt.savefig(output_plot, dpi=300)
        print(f"\nPlot visualisasi gabungan berhasil disimpan ke: {output_plot}")
        
    except Exception as e:
        print(f"\nGagal membuat visualisasi: {e}")

if __name__ == "__main__":
    main()

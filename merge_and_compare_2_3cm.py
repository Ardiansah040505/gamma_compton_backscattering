#!/usr/bin/env python3
import os
import glob
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def main():
    build_dir = "/data/mahasiswa/ardian/gcb_project/gcb_ver/gamma_compton_backscattering/build"
    hasil_dir = "/data/mahasiswa/ardian/gcb_project/gcb_ver/hasil_sementara"
    name_tag = "crack2_3cm"
    title_tag = "Crack 2.3cm"
    
    print(f"\n{'='*70}")
    print(f" Memproses Simulasi {title_tag} ".center(70, "="))
    print(f"{'='*70}")
    
    csv_files = glob.glob(os.path.join(build_dir, "scan_results_*.csv"))
    filtered_csv_files = [f for f in csv_files if os.path.getsize(f) > 1000]
    
    print(f"Menemukan total {len(csv_files)} file CSV.")
    print(f"Menggunakan {len(filtered_csv_files)} file CSV setelah menyaring.")
    
    if not filtered_csv_files:
        print(f"Error: Tidak ada file CSV valid ditemukan di {build_dir}")
        return
        
    df_list = []
    for file in sorted(filtered_csv_files):
        df = pd.read_csv(file)
        df_list.append(df)
        
    merged_df = pd.concat(df_list, ignore_index=True)
    merged_df.sort_values(by=['x', 'y', 'detectorID'], inplace=True)
    
    # Save to local build folder as merged_scan_results.csv
    local_merged_path = os.path.join(build_dir, "merged_scan_results.csv")
    merged_df.to_csv(local_merged_path, index=False)
    print(f"Berhasil menyimpan hasil merge lokal ke: {local_merged_path}")
    
    # Save as hasil_sementara/crack2_3cm.csv
    output_crack_path = os.path.join(hasil_dir, f"{name_tag}.csv")
    merged_df.to_csv(output_crack_path, index=False)
    print(f"Berhasil menyimpan hasil merge ke: {output_crack_path}")
    
    # 2. Compare with normal2.csv
    normal_path = os.path.join(hasil_dir, "normal2.csv")
    if not os.path.exists(normal_path):
        print(f"Error: File '{normal_path}' tidak ditemukan.")
        return
        
    print(f"Membaca data normal dari: {normal_path}...")
    df_normal = pd.read_csv(normal_path)
    
    val_cols = ['roiCounts', 'normalizedCounts', 'flux', 'normalizedFlux']
    
    df_normal_combined = df_normal.groupby(['x', 'y'])[val_cols].sum().reset_index()
    df_crack_combined = merged_df.groupby(['x', 'y'])[val_cols].sum().reset_index()
    
    merged = pd.merge(df_crack_combined, df_normal_combined, on=['x', 'y'], suffixes=('_crack', '_normal'))
    
    if merged.empty:
        print("Error: Tidak ada data yang cocok setelah penggabungan koordinat.")
        return
        
    print(f"Berhasil menyelaraskan {len(merged)} koordinat (x, y) untuk perbandingan.")
    
    for col in val_cols:
        col_crack = f"{col}_crack"
        col_normal = f"{col}_normal"
        
        merged[f'{col}_abs_diff'] = merged[col_crack] - merged[col_normal]
        
        merged[f'{col}_rel_diff'] = np.where(
            merged[col_normal] != 0,
            (merged[col_crack] - merged[col_normal]) / merged[col_normal],
            0.0
        )
        
        denom = (merged[col_crack] + merged[col_normal]) / 2
        merged[f'{col}_sym_rel_diff'] = np.where(
            denom != 0,
            (merged[col_crack] - merged[col_normal]) / denom,
            0.0
        )
        
    output_diff_csv = os.path.join(hasil_dir, f"crack_normal_combined_relative_diff_{name_tag}.csv")
    merged.to_csv(output_diff_csv, index=False)
    print(f"Hasil perhitungan gabungan berhasil disimpan ke: {output_diff_csv}")
    
    print("\n" + "="*70)
    print(f" RINGKASAN PERBANDINGAN GABUNGAN DETEKTOR ({title_tag} vs Normal) ".center(70, "="))
    print(" Formula: (Crack_Total - Normal_Total) / Normal_Total ")
    print("="*70)
    
    for col in val_cols:
        rel_col = f'{col}_rel_diff'
        abs_col = f'{col}_abs_diff'
        
        rel_diffs = merged[rel_col] * 100
        abs_diffs = merged[abs_col]
        
        print(f"\nKolom: {col}")
        print(f"  * Perbedaan Absolut : Rata-rata = {abs_diffs.mean():+.6f}, Rentang = [{abs_diffs.min():+.6f}, {abs_diffs.max():+.6f}]")
        print(f"  * Perbedaan Relatif : Rata-rata = {rel_diffs.mean():+.4f}%, Standar Deviasi = {rel_diffs.std():.4f}%")
        print(f"  * Rentang Persen    : [{rel_diffs.min():+.4f}%, {rel_diffs.max():+.4f}%]")
    print("="*70)
    
    try:
        fig, axes = plt.subplots(1, 2, figsize=(16, 7))
        targets = ['roiCounts_rel_diff', 'normalizedFlux_rel_diff']
        titles = [f'Relative Difference of Combined roiCounts (%)\n({title_tag} vs Normal)', 
                  f'Relative Difference of Combined normalizedFlux (%)\n({title_tag} vs Normal)']
        
        x_unique = np.sort(merged['x'].unique())
        y_unique = np.sort(merged['y'].unique())
        
        for idx, target_col in enumerate(targets):
            grid_diff = merged.pivot(index='y', columns='x', values=target_col)
            
            im = axes[idx].imshow(
                grid_diff * 100, 
                extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                origin='lower', 
                cmap='RdBu_r', 
                aspect='equal',
                interpolation='bilinear'
            )
            axes[idx].set_title(titles[idx], fontsize=12)
            axes[idx].set_xlabel("X (cm)")
            axes[idx].set_ylabel("Y (cm)")
            fig.colorbar(im, ax=axes[idx], label="Difference (%)")
            
        plt.suptitle(f"Peta Perbedaan Relatif Gabungan Detektor ({title_tag} vs Normal)", fontsize=14, y=0.98)
        plt.tight_layout(rect=[0, 0.05, 1, 0.95])
        
        output_plot = os.path.join(hasil_dir, f"crack_normal_combined_relative_diff_map_{name_tag}.png")
        plt.savefig(output_plot, dpi=300, bbox_inches='tight')
        plt.close(fig)
        print(f"\nPlot visualisasi gabungan berhasil disimpan ke: {output_plot}")
        
        fig_zoom, axes_zoom = plt.subplots(1, 2, figsize=(16, 9))
        x_ticks_zoom = np.arange(-5, 6, 1)
        y_ticks_zoom = np.arange(15, 31, 1)
        
        for idx, target_col in enumerate(targets):
            grid_diff = merged.pivot(index='y', columns='x', values=target_col)
            
            im = axes_zoom[idx].imshow(
                grid_diff * 100, 
                extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()],
                origin='lower', 
                cmap='RdBu_r', 
                aspect='equal',
                interpolation='bilinear'
            )
            axes_zoom[idx].set_title(titles[idx] + " (Zoom Area)", fontsize=12)
            axes_zoom[idx].set_xlabel("X (cm)")
            axes_zoom[idx].set_ylabel("Y (cm)")
            
            axes_zoom[idx].set_xlim(-5, 5)
            axes_zoom[idx].set_ylim(15, 30)
            
            axes_zoom[idx].set_xticks(x_ticks_zoom)
            axes_zoom[idx].set_yticks(y_ticks_zoom)
            axes_zoom[idx].grid(True, which='both', color='gray', linestyle='--', linewidth=0.5, alpha=0.5)
            
            fig_zoom.colorbar(im, ax=axes_zoom[idx], label="Difference (%)")
            
        plt.suptitle(f"Peta Detail (Zoom-In 1cm Scale) Perbedaan Relatif ({title_tag} vs Normal)", fontsize=14, y=0.98)
        plt.tight_layout(rect=[0, 0.05, 1, 0.95])
        
        output_plot_zoom = os.path.join(hasil_dir, f"crack_normal_combined_relative_diff_map_{name_tag}_zoom.png")
        plt.savefig(output_plot_zoom, dpi=300, bbox_inches='tight')
        plt.close(fig_zoom)
        print(f"Plot zoom-in berhasil disimpan ke: {output_plot_zoom}")
        
    except Exception as e:
        print(f"\nGagal membuat visualisasi: {e}")

if __name__ == "__main__":
    main()

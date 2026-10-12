#!/usr/bin/env python3
import os
import shutil
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from multiprocessing import Pool
from PIL import Image

# Folder paths
hasil_dir = "/data/mahasiswa/ardian/gcb_project/gcb_ver/hasil_sementara"
csv_path = os.path.join(hasil_dir, "crack_normal_combined_relative_diff_crack2cm.csv")
tmp_frames_dir = "/data/mahasiswa/ardian/gcb_project/gcb_ver/hasil_sementara/tmp_frames"

def render_frame(args):
    frame_idx, end_idx, df_sorted_path, limit, x_unique, y_unique, x_min, x_max, y_min, y_max = args
    
    # Load df_sorted
    df_sorted = pd.read_csv(df_sorted_path)
    
    # Buat copy data dan set nilai titik yang belum terpindai menjadi NaN
    df_temp = df_sorted.copy()
    if end_idx < len(df_temp):
        df_temp.loc[end_idx:, 'roiCounts_rel_diff'] = np.nan
        
    # Pivot agar menjadi grid 2D (shape akan selalu sama karena koordinat x dan y lengkap)
    grid_diff = df_temp.pivot(index='y', columns='x', values='roiCounts_rel_diff')
    
    fig, ax = plt.subplots(figsize=(10, 8))
    
    # Gunakan imshow dengan interpolasi bilinear persis seperti di grafik PNG asli
    im = ax.imshow(
        grid_diff * 100, 
        extent=[x_min, x_max, y_min, y_max],
        origin='lower', 
        cmap='RdBu_r', 
        vmin=-limit, 
        vmax=limit,
        aspect='equal',
        interpolation='bilinear'
    )
    
    ax.set_xlim(x_min - 2, x_max + 2)
    ax.set_ylim(y_min - 2, y_max + 2)
    ax.set_xlabel("X (cm)", fontsize=11)
    ax.set_ylabel("Y (cm)", fontsize=11)
    
    percent_done = (end_idx / len(df_sorted)) * 100
    ax.set_title(f"Animasi Pemindaian Heatmap (Crack 2cm - roiCounts)\nProgress: {percent_done:.1f}% ({end_idx}/{len(df_sorted)} points)", fontsize=12, pad=15)
    
    fig.colorbar(im, ax=ax, label="Difference (%)")
    ax.grid(True, which='both', color='gray', linestyle='--', linewidth=0.5, alpha=0.3)
    
    # Save frame
    frame_path = os.path.join(tmp_frames_dir, f"frame_{frame_idx:04d}.png")
    plt.savefig(frame_path, dpi=100, bbox_inches='tight')
    plt.close(fig)
    return frame_path

def main():
    if not os.path.exists(csv_path):
        print(f"Error: File '{csv_path}' tidak ditemukan.")
        return

    print("Membaca data...")
    df = pd.read_csv(csv_path)
    
    # Urutkan koordinat agar animasinya mengalir alami (dari bawah ke atas)
    df_sorted = df.sort_values(by=['y', 'x']).reset_index(drop=True)
    
    # Simpan sementara df_sorted ke csv untuk dibaca worker
    df_sorted_temp_path = os.path.join(hasil_dir, "temp_sorted_animation.csv")
    df_sorted.to_csv(df_sorted_temp_path, index=False)
    
    x_unique = np.sort(df_sorted['x'].unique())
    y_unique = np.sort(df_sorted['y'].unique())
    x_min, x_max = x_unique.min(), x_unique.max()
    y_min, y_max = y_unique.min(), y_unique.max()
    
    val_min = df_sorted['roiCounts_rel_diff'].min() * 100
    val_max = df_sorted['roiCounts_rel_diff'].max() * 100
    limit = min(max(abs(val_min), abs(val_max)), 15.0)
    
    # Setup temporary directory for frames
    if os.path.exists(tmp_frames_dir):
        shutil.rmtree(tmp_frames_dir)
    os.makedirs(tmp_frames_dir)
    
    # step_size = 101 (1 baris scan penuh per frame)
    step_size = 101  
    num_frames = (len(df_sorted) + step_size - 1) // step_size
    
    print(f"Mempersiapkan parameter untuk {num_frames} frames...")
    tasks = []
    for idx in range(num_frames):
        end_idx = min((idx + 1) * step_size, len(df_sorted))
        tasks.append((idx, end_idx, df_sorted_temp_path, limit, x_unique, y_unique, x_min, x_max, y_min, y_max))
        
    num_cpus = os.cpu_count()
    print(f"Mulai merender frame menggunakan {num_cpus} CPU cores secara paralel...")
    
    with Pool(processes=num_cpus) as pool:
        frame_paths = pool.map(render_frame, tasks)
        
    print("Semua frame berhasil dirender. Mengompilasi menjadi GIF...")
    
    images = [Image.open(fp) for fp in sorted(frame_paths)]
    output_gif = os.path.join(hasil_dir, "scanning_crack_2cm_roiCounts_full.gif")
    
    images[0].save(
        output_gif,
        save_all=True,
        append_images=images[1:],
        optimize=True,
        duration=100,  # 100 ms per frame (10 FPS)
        loop=0
    )
    
    # Clean up
    if os.path.exists(tmp_frames_dir):
        shutil.rmtree(tmp_frames_dir)
    if os.path.exists(df_sorted_temp_path):
        os.remove(df_sorted_temp_path)
        
    print(f"Selesai! Animasi Full Area berhasil disimpan di: {output_gif}")

if __name__ == "__main__":
    main()

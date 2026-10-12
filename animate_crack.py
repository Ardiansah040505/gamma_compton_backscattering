#!/usr/bin/env python3
import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, PillowWriter

def main():
    hasil_dir = "/data/mahasiswa/ardian/gcb_project/gcb_ver/hasil_sementara"
    csv_path = os.path.join(hasil_dir, "crack_normal_combined_relative_diff_crack2cm.csv")
    
    if not os.path.exists(csv_path):
        print(f"Error: File '{csv_path}' tidak ditemukan.")
        return

    print(f"Membaca data dari: {csv_path}...")
    df = pd.read_csv(csv_path)
    
    # Urutkan koordinat agar animasinya mengalir alami (raster scan: dari Y bawah ke atas, X kiri ke kanan)
    df_sorted = df.sort_values(by=['y', 'x']).reset_index(drop=True)
    
    print(f"Total titik yang akan dianimasikan: {len(df_sorted)}")
    
    # Persiapkan Figure dan Axes
    fig, ax = plt.subplots(figsize=(10, 8))
    
    # Fokus area zoom ke lokasi retakan (X: -5 s/d 5, Y: 15 s/d 30)
    ax.set_xlim(-5, 5)
    ax.set_ylim(15, 30)
    ax.set_xlabel("X (cm)", fontsize=11)
    ax.set_ylabel("Y (cm)", fontsize=11)
    ax.set_title("Animasi Pemindaian Heatmap Scatter (Crack 2cm - roiCounts)", fontsize=13, pad=15)
    
    # Menggunakan grid pembantu
    x_ticks = np.arange(-5, 6, 1)
    y_ticks = np.arange(15, 31, 1)
    ax.set_xticks(x_ticks)
    ax.set_yticks(y_ticks)
    ax.grid(True, which='both', color='gray', linestyle='--', linewidth=0.5, alpha=0.5)
    
    # Inisialisasi scatter plot kosong
    val_min = df_sorted['roiCounts_rel_diff'].min() * 100
    val_max = df_sorted['roiCounts_rel_diff'].max() * 100
    limit = max(abs(val_min), abs(val_max))
    limit = min(limit, 15.0)  # Batas visualisasi
    
    scatter = ax.scatter([], [], c=[], cmap='RdBu_r', vmin=-limit, vmax=limit, s=70, edgecolors='none', marker='s')
    
    # Tambahkan colorbar
    fig.colorbar(scatter, ax=ax, label="Relative Difference (%)")
    
    # Elemen teks untuk menampilkan jumlah titik/progress
    progress_text = ax.text(0.02, 0.95, '', transform=ax.transAxes, fontsize=10, 
                            bbox=dict(facecolor='white', alpha=0.7, edgecolor='none'))

    # Mengoptimalkan frame: menambahkan 50 titik per frame agar pembuatan GIF jauh lebih cepat dan lancar
    step_size = 50  
    num_frames = (len(df_sorted) + step_size - 1) // step_size

    def init():
        scatter.set_offsets(np.empty((0, 2)))
        scatter.set_array(np.empty((0,)))
        progress_text.set_text('')
        return scatter, progress_text

    def update(frame):
        end_idx = min((frame + 1) * step_size, len(df_sorted))
        data_slice = df_sorted.iloc[:end_idx]
        
        # Update posisi koordinat titik
        offsets = np.c_[data_slice['x'], data_slice['y']]
        scatter.set_offsets(offsets)
        
        # Update warna heatmap (%)
        colors = data_slice['roiCounts_rel_diff'] * 100
        scatter.set_array(colors)
        
        # Update teks progress
        percent_done = (end_idx / len(df_sorted)) * 100
        progress_text.set_text(f"Scanning: {percent_done:.1f}% ({end_idx}/{len(df_sorted)} points)")
        
        return scatter, progress_text

    print(f"Membuat animasi dengan {num_frames} frames (step_size={step_size})...")
    ani = FuncAnimation(fig, update, frames=num_frames, init_func=init, blit=True, interval=100)
    
    output_gif = os.path.join(hasil_dir, "scanning_crack_2cm_roiCounts.gif")
    print(f"Menyimpan animasi ke: {output_gif} (menggunakan Pillow)...")
    
    writer = PillowWriter(fps=10)
    ani.save(output_gif, writer=writer)
    print("Selesai! Animasi berhasil disimpan.")

if __name__ == "__main__":
    main()

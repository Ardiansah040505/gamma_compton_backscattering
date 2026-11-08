import os
import glob
import re
import pandas as pd
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def load_and_aggregate(csv_file):
    """
    Membaca berkas CSV dan menjumlahkan cacahan dari seluruh detektor
    untuk setiap titik koordinat posisi pemindaian (x, y).
    """
    df = pd.read_csv(csv_file)
    grid = df.groupby(['x', 'y'])[['roiCounts', 'flux']].sum().reset_index()
    return grid

def load_and_preprocess(crack_file, normal_input):
    """
    Menggabungkan data pemindaian retakan dengan data acuan normal.
    normal_input dapat berupa path berkas CSV normal atau DataFrame yang sudah diagregasi.
    """
    grid_crack = load_and_aggregate(crack_file) if isinstance(crack_file, str) else crack_file

    if isinstance(normal_input, str):
        grid_normal = load_and_aggregate(normal_input)
    else:
        grid_normal = normal_input

    merged = pd.merge(grid_crack, grid_normal, on=['x', 'y'], suffixes=('_crack', '_normal'))
    return merged

def parse_crack_width(filename):
    """
    Mengekstrak nilai lebar retakan (cm) dari nama berkas untuk pengurutan numerik.
    Contoh: crack0_1cm.csv -> 0.1, crack1_3cm.csv -> 1.3, crack1cm.csv -> 1.0, crack2cm.csv -> 2.0
    """
    clean_name = os.path.basename(filename).replace('.csv', '')
    if clean_name == 'crack1':
        return 1.0
    if clean_name == 'crack2':
        return 2.0
    s = clean_name.replace('crack', '').replace('cm', '').replace('_', '.')
    try:
        return float(s)
    except ValueError:
        return 999.0

def find_crack_files(data_dir='.'):
    """
    Mendeteksi seluruh file simulasi retakan (crack*.csv),
    mengabaikan file turunan, dan menyaring duplikat identik.
    """
    all_files = glob.glob(os.path.join(data_dir, 'crack*.csv'))
    valid_files = [
        f for f in all_files 
        if not os.path.basename(f).startswith(('crack_normal', 'crack_fwhm'))
    ]

    # Kelompokkan dan saring duplikasi ukuran (misal crack1.csv vs crack1cm.csv)
    file_map = {}
    for f in valid_files:
        fname = os.path.basename(f)
        w = parse_crack_width(fname)
        # Prioritaskan format nama dengan 'cm'
        if w not in file_map or ('cm' in fname and 'cm' not in file_map[w][0]):
            file_map[w] = (fname, f)

    # Urutkan berdasarkan lebar retakan secara numerik
    sorted_items = sorted(file_map.items(), key=lambda item: item[0])
    crack_files = [(item[1][1], item[1][0], item[0]) for item in sorted_items]
    return crack_files

def plot_line_profile(merged_df, crack_tag='Crack', x_slice=0, save_path='line_profile.png'):
    """
    Mengekstrak dan menyimpan grafik profil garis vertikal pada sumbu Y untuk nilai X tertentu.
    """
    profile = merged_df[merged_df['x'] == x_slice].sort_values(by='y')

    fig, ax = plt.subplots(figsize=(9, 4.5), dpi=150)
    ax.plot(profile['y'], profile['roiCounts_normal'], label='Normal (roiCounts)', color='#0055ff', linestyle='--')
    ax.plot(profile['y'], profile['roiCounts_crack'], label=f'{crack_tag} (roiCounts)', color='#d62728', linewidth=2)
    
    ax.set_title(f'Profil Garis Vertikal Melintasi {crack_tag} (X = {x_slice} cm)', fontsize=12, fontweight='bold')
    ax.set_xlabel('Posisi Vertikal Y (cm)', fontsize=11)
    ax.set_ylabel('Total roiCounts (Gabungan Detektor)', fontsize=11)
    ax.grid(True, linestyle=':', alpha=0.6)
    ax.legend(frameon=True)
    plt.tight_layout()
    plt.savefig(save_path)
    plt.close(fig)

def compute_cnr_table(merged_df, crack_tag=None, actual_w=None, y_crack=0, x_range=(-20, 20)):
    """
    Menghitung tabel CNR otomatis berdasarkan definisi Persamaan:
    CNR = |u_defect - u_background| / sigma_background
    """
    # 1. ROI Cacat (Defect) retakan horizontal pada Y=0, X melintang [-20, 20]
    crack_roi = merged_df[(merged_df['y'] == y_crack) & 
                          (merged_df['x'] >= x_range[0]) & 
                          (merged_df['x'] <= x_range[1])]

    # 2. ROI Normal Berpasangan (Paired Background pada koordinat yang sama)
    normal_paired_roi = crack_roi

    # 3. ROI Latar Spasial Homogen (di luar area retakan |y| >= 20)
    bg_spatial_roi = merged_df[merged_df['y'].abs() >= 20]

    records = []
    for metric in ['roiCounts', 'flux']:
        u_defect = crack_roi[f'{metric}_crack'].mean()
        
        # Pendekatan A: Dibandingkan terhadap model homogen tanpa cacat (Template Normal)
        u_norm_paired = normal_paired_roi[f'{metric}_normal'].mean()
        sigma_norm_paired = normal_paired_roi[f'{metric}_normal'].std()
        cnr_paired = abs(u_defect - u_norm_paired) / sigma_norm_paired if sigma_norm_paired > 0 else np.nan

        # Pendekatan B: Dibandingkan terhadap latar spasial lateral pada file yang sama
        u_bg_spatial = bg_spatial_roi[f'{metric}_crack'].mean()
        sigma_bg_spatial = bg_spatial_roi[f'{metric}_crack'].std()
        cnr_spatial = abs(u_defect - u_bg_spatial) / sigma_bg_spatial if sigma_bg_spatial > 0 else np.nan

        rec = {}
        if crack_tag is not None:
            rec['Crack'] = crack_tag
        if actual_w is not None:
            rec['Actual_Width_cm'] = actual_w

        rec.update({
            'Metrik': metric,
            'Defect Mean (u_i)': round(u_defect, 2),
            'Normal Mean (u_b)': round(u_norm_paired, 2),
            'Normal Std (sigma_b)': round(sigma_norm_paired, 2),
            'CNR (vs Normal)': round(cnr_paired, 2),
            'Spatial BG Mean': round(u_bg_spatial, 2),
            'Spatial BG Std': round(sigma_bg_spatial, 2),
            'CNR (Spasial Latar)': round(cnr_spatial, 2)
        })
        records.append(rec)

    return pd.DataFrame(records)

def plot_cnr_trend(summary_df, save_path):
    """
    Membuat grafik komparasi nilai CNR vs Lebar Retakan untuk seluruh dataset.
    """
    fig, axes = plt.subplots(1, 2, figsize=(14, 5.5), dpi=150)

    for idx, metric in enumerate(['roiCounts', 'flux']):
        ax = axes[idx]
        subset = summary_df[summary_df['Metrik'] == metric].sort_values(by='Actual_Width_cm')

        ax.plot(subset['Actual_Width_cm'], subset['CNR (vs Normal)'], marker='o', linewidth=2, 
                color='#1f77b4', label='CNR (vs Template Normal)')
        ax.plot(subset['Actual_Width_cm'], subset['CNR (Spasial Latar)'], marker='s', linewidth=2, 
                linestyle='--', color='#ff7f0e', label='CNR (Spasial Latar)')

        ax.set_title(f'Tren CNR vs Lebar Retakan - Metrik: {metric}', fontsize=12, fontweight='bold')
        ax.set_xlabel('Lebar Retakan Sebenarnya (cm)', fontsize=11)
        ax.set_ylabel('Contrast-to-Noise Ratio (CNR)', fontsize=11)
        ax.grid(True, linestyle=':', alpha=0.6)
        ax.legend(frameon=True)

    plt.tight_layout()
    plt.savefig(save_path)
    plt.close(fig)

def main():
    output_dir = 'hasil_cnr'
    os.makedirs(output_dir, exist_ok=True)

    file_normal = 'normal2.csv'
    if not os.path.exists(file_normal):
        print(f"[ERROR] Berkas normal acuan '{file_normal}' tidak ditemukan!")
        return

    print(f"[INFO] Membaca dan mengagregasi data acuan: {file_normal} ...")
    grid_normal = load_and_aggregate(file_normal)

    # Deteksi semua file keretakan
    crack_files = find_crack_files('.')
    if not crack_files:
        print("[ERROR] Tidak ditemukan berkas retakan 'crack*.csv'!")
        return

    print(f"[INFO] Ditemukan {len(crack_files)} dataset keretakan. Memulai pemrosesan batch...\n")

    all_cnr_records = []

    for idx, (file_path, file_name, width_cm) in enumerate(crack_files, start=1):
        crack_tag = file_name.replace('.csv', '')
        print(f"[{idx}/{len(crack_files)}] Memproses {crack_tag} (Lebar: {width_cm:.2f} cm)...")

        # 1. Gabungkan data
        merged_data = load_and_preprocess(file_path, grid_normal)

        # 2. Plot dan simpan Vertical Line Profile pada X = 0
        profile_img_name = f"line_profile_{crack_tag}_x0.png"
        profile_img_path = os.path.join(output_dir, profile_img_name)
        plot_line_profile(merged_data, crack_tag=crack_tag, x_slice=0, save_path=profile_img_path)

        # 3. Hitung tabel evaluasi CNR (retakan horizontal pada Y=0)
        cnr_df = compute_cnr_table(merged_data, crack_tag=crack_tag, actual_w=width_cm, y_crack=0, x_range=(-20, 20))
        all_cnr_records.append(cnr_df)

    # Gabungkan seluruh hasil CNR ke dalam satu DataFrame rekap
    summary_df = pd.concat(all_cnr_records, ignore_index=True)

    # 4. Simpan rekapitulasi ke file CSV
    summary_csv_path = os.path.join(output_dir, 'cnr_summary_all_cracks.csv')
    summary_df.to_csv(summary_csv_path, index=False)
    print(f"\n[INFO] Tabel rekapitulasi CNR berhasil disimpan ke: {summary_csv_path}")

    # 5. Buat dan simpan grafik tren komparasi CNR
    trend_img_path = os.path.join(output_dir, 'cnr_vs_crack_width.png')
    plot_cnr_trend(summary_df, trend_img_path)
    print(f"[INFO] Grafik tren CNR vs Lebar Retakan disimpan ke: {trend_img_path}")

    # 6. Jalankan makro ROOT C untuk grafik standar ROOT
    print("\n[INFO] Menjalankan makro ROOT C: cnr_vs_width_plot.C ...")
    os.system("root -l -b -q cnr_vs_width_plot.C")

    # 7. Tampilkan tabel ringkasan ke terminal
    print("\n" + "=" * 115)
    print(" RINGKASAN EVALUASI CNR SELURUH RETAKAN PROYEKSI CBST ".center(115, "="))
    print("=" * 115)
    print(summary_df.to_string(index=False))
    print("=" * 115)
    print(f"[SELESAI] Seluruh output ({len(crack_files)} grafik profil, 1 tabel rekap, 1 grafik tren) terkumpul di folder: {output_dir}/\n")

if __name__ == '__main__':
    main()

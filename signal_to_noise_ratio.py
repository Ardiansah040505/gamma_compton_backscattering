import os
import glob
import pandas as pd
import numpy as np

def parse_crack_size(filename):
    """
    Ekstraksi ukuran retakan (float, cm) dari nama berkas secara fleksibel.
    Mendukung: crack0_1cm.csv, crack1.csv, crack1cm.csv, crack1_3cm.csv, crack2cm.csv
    """
    clean = os.path.basename(filename).replace('.csv', '')
    if clean == 'crack1':
        return 1.0
    if clean == 'crack2':
        return 2.0
    s = clean.replace('crack', '').replace('cm', '').replace('_', '.')
    try:
        return float(s)
    except ValueError:
        return 999.0

def find_valid_crack_files(data_dir='.'):
    """
    Mendeteksi seluruh file simulasi mentah keretakan,
    mengabaikan file turunan dan menyaring duplikasi nama file.
    """
    all_matched = glob.glob(os.path.join(data_dir, "crack*.csv"))
    raw_files = [
        f for f in all_matched
        if not os.path.basename(f).startswith(('crack_normal', 'crack_fwhm'))
    ]

    file_dict = {}
    for f in raw_files:
        size = parse_crack_size(f)
        fname = os.path.basename(f)
        if size not in file_dict or ('cm' in fname and 'cm' not in file_dict[size]):
            file_dict[size] = f

    sorted_sizes = sorted(file_dict.keys())
    return [(s, file_dict[s]) for s in sorted_sizes if s < 900.0]

def load_and_aggregate_grid(file_or_df):
    """
    Membaca dan mengagregasi cacahan detektor per koordinat spasial (x, y).
    """
    if isinstance(file_or_df, str):
        df = pd.read_csv(file_or_df)
    else:
        df = file_or_df
    grid = df.groupby(['x', 'y'])[['roiCounts', 'flux']].sum().reset_index()
    return grid

def calculate_snr_roi(grid_df, x_range, y_range, metric='roiCounts'):
    """
    Menghitung SNR = mu / sigma berdasarkan Persamaan (5) Barrientos et al. (2026).
    """
    roi_data = grid_df[(grid_df['x'] >= x_range[0]) & (grid_df['x'] <= x_range[1]) &
                       (grid_df['y'] >= y_range[0]) & (grid_df['y'] <= y_range[1])][metric]

    mu    = float(roi_data.mean()) if len(roi_data) > 0 else 0.0
    sigma = float(roi_data.std())  if len(roi_data) > 1 else 0.0
    snr   = (mu / sigma) if sigma > 0 else 0.0

    return {'mu': mu, 'sigma': sigma, 'snr': snr}

def main():
    output_dir = 'hasil_snr'
    os.makedirs(output_dir, exist_ok=True)

    file_normal = 'normal2.csv'
    if not os.path.exists(file_normal):
        print(f"[ERROR] Berkas acuan '{file_normal}' tidak ditemukan!")
        return

    # 1. Agregasi acuan normal 1 kali di memori
    print(f"[INFO] Membaca data acuan normal: {file_normal} ...")
    grid_normal = load_and_aggregate_grid(file_normal)

    # Koordinat ROI cacat / tengah untuk retakan horizontal: Y in [-1, 1], X in [-20, 20]
    x_defect_range = (-20, 20)
    y_defect_range = (-1, 1)

    # Hitung baseline SNR normal
    snr_norm_counts = calculate_snr_roi(grid_normal, x_defect_range, y_defect_range, metric='roiCounts')
    snr_norm_flux   = calculate_snr_roi(grid_normal, x_defect_range, y_defect_range, metric='flux')

    # 2. Deteksi semua file keretakan yang valid
    crack_items = find_valid_crack_files('.')
    if not crack_items:
        print("[ERROR] Tidak ditemukan berkas keretakan yang valid!")
        return

    print(f"[INFO] Ditemukan {len(crack_items)} dataset keretakan. Memulai analisis batch SNR...")

    results = []

    for size_val, file_path in crack_items:
        fname      = os.path.basename(file_path)
        grid_crack = load_and_aggregate_grid(file_path)

        for metric in ['roiCounts', 'flux']:
            norm_stats   = snr_norm_counts if metric == 'roiCounts' else snr_norm_flux
            defect_stats = calculate_snr_roi(grid_crack, x_defect_range, y_defect_range, metric=metric)

            snr_ratio = (defect_stats['snr'] / norm_stats['snr']) if norm_stats['snr'] > 0 else np.nan

            results.append({
                'File':              fname,
                'Crack_Size_cm':     size_val,
                'Metric':            metric,
                'Defect_Mean':       round(defect_stats['mu'],    2),
                'Defect_Std':        round(defect_stats['sigma'], 2),
                'Defect_SNR':        round(defect_stats['snr'],   2),
                'Normal_Mean':       round(norm_stats['mu'],      2),
                'Normal_Std':        round(norm_stats['sigma'],   2),
                'Normal_SNR':        round(norm_stats['snr'],     2),
                'SNR_Ratio_to_Normal': round(snr_ratio, 3)
            })

    df_summary = pd.DataFrame(results).sort_values(by=['Metric', 'Crack_Size_cm'])

    # 3. Simpan tabel ringkasan ke file CSV di folder hasil_snr/
    csv_summary_path = os.path.join(output_dir, 'snr_summary_all_cracks.csv')
    df_summary.to_csv(csv_summary_path, index=False)
    print(f"\n[INFO] Tabel rekapitulasi SNR disimpan ke: {csv_summary_path}")

    # 4. Tampilkan ringkasan metrik roiCounts di terminal
    print("\n" + "=" * 90)
    print(" REKAPITULASI EVALUASI SNR SELURUH RETAKAN (roiCounts) ".center(90, "="))
    print("=" * 90)
    counts_table = df_summary[df_summary['Metric'] == 'roiCounts'][
        ['File', 'Crack_Size_cm', 'Defect_Mean', 'Defect_Std', 'Defect_SNR', 'Normal_SNR', 'SNR_Ratio_to_Normal']
    ]
    print(counts_table.to_string(index=False))
    print("=" * 90)

    # 5. Panggil makro ROOT C untuk membuat grafik
    print("\n[INFO] Membuat grafik SNR menggunakan makro ROOT ...")
    os.system("root -l -b -q snr_plot.C")

    print(f"[SELESAI] Seluruh hasil SNR tersimpan rapi di direktori: {output_dir}/\n")

if __name__ == '__main__':
    main()

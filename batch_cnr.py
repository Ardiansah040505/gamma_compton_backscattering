import os
import glob
import pandas as pd
import numpy as np

def parse_crack_size(filename):
    """
    Ekstraksi ukuran retakan (float, cm) dari nama berkas secara fleksibel.
    Mendukung format: crack0_1cm.csv, crack1.csv, crack1cm.csv, crack1_3cm.csv, crack2cm.csv
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
    Mendeteksi hanya file simulasi mentah keretakan,
    mengabaikan file turunan dan menyaring duplikasi data.
    """
    all_matched = glob.glob(os.path.join(data_dir, "crack*.csv"))
    raw_files = [
        f for f in all_matched
        if not os.path.basename(f).startswith(('crack_normal', 'crack_fwhm'))
    ]

    # Deduplikasi (misal crack1.csv vs crack1cm.csv)
    file_dict = {}
    for f in raw_files:
        size = parse_crack_size(f)
        fname = os.path.basename(f)
        if size not in file_dict or ('cm' in fname and 'cm' not in file_dict[size]):
            file_dict[size] = f

    sorted_sizes = sorted(file_dict.keys())
    return [(s, file_dict[s]) for s in sorted_sizes if s < 900.0]

def export_line_profiles_csv(norm_line_sub, crack_profiles, cnr_map, out_csv_path):
    """
    Mengekspor irisan profil garis horizontal (Y=0, X in [-5, 5])
    ke berkas CSV agar dapat dibaca oleh makro ROOT C.
    """
    target_samples = [0.1, 0.3, 0.5, 1.0, 1.5, 2.0, 2.5]
    plotted_keys = []
    for ts in target_samples:
        closest = min(crack_profiles.keys(), key=lambda k: abs(k - ts))
        if closest not in plotted_keys:
            plotted_keys.append(closest)
    plotted_keys.sort()

    records = []
    # 1. Normal baseline
    for _, row in norm_line_sub.iterrows():
        records.append({
            'Sample': 'Normal',
            'Crack_Size_cm': 0.0,
            'CNR': 0.0,
            'x': round(float(row['x']), 4),
            'roiCounts': round(float(row['roiCounts']), 2)
        })

    # 2. Defect samples
    for size in plotted_keys:
        prof = crack_profiles[size]
        cnr_val = cnr_map.get(size, 0.0)
        s_label = f"Crack {size:g} cm"
        for _, row in prof.iterrows():
            records.append({
                'Sample': s_label,
                'Crack_Size_cm': size,
                'CNR': round(cnr_val, 2),
                'x': round(float(row['x']), 4),
                'roiCounts': round(float(row['roiCounts']), 2)
            })

    df_out = pd.DataFrame(records)
    df_out.to_csv(out_csv_path, index=False)
    print(f"[INFO] Data profil garis diekspor ke: {out_csv_path}")

def main():
    output_dir = 'hasil_batch_cnr'
    os.makedirs(output_dir, exist_ok=True)

    file_normal = 'normal2.csv'
    if not os.path.exists(file_normal):
        print(f"[ERROR] Reference file '{file_normal}' not found!")
        return

    # 1. Baca data normal sebagai acuan (caching 1 kali di memori)
    print(f"[INFO] Loading normal reference data: {file_normal} ...")
    df_normal = pd.read_csv(file_normal)
    grid_norm = df_normal.groupby(['x', 'y'])[['roiCounts', 'flux']].sum().reset_index()

    # Normal line profile at Y = 0 cm (center ROI -5 <= X <= 5)
    norm_line = grid_norm[grid_norm['y'] == 0].sort_values(by='x')
    norm_line_sub = norm_line[(norm_line['x'] >= -5) & (norm_line['x'] <= 5)]

    # Normal paired ROI at defect location (X=0, Y in [-20, 20])
    norm_roi = grid_norm[(grid_norm['x'] == 0) & (grid_norm['y'] >= -20) & (grid_norm['y'] <= 20)]
    u_norm = norm_roi['roiCounts'].mean()
    std_norm = norm_roi['roiCounts'].std()

    # 2. Cari seluruh file crack yang valid
    crack_items = find_valid_crack_files('.')
    if not crack_items:
        print("[ERROR] No valid crack files found!")
        return

    print(f"[INFO] Found {len(crack_items)} valid crack datasets. Running batch analysis...")

    results = []
    crack_profiles = {}

    for size_val, file_path in crack_items:
        df_c = pd.read_csv(file_path)
        grid_c = df_c.groupby(['x', 'y'])[['roiCounts', 'flux']].sum().reset_index()

        # Defect ROI (X=0, Y in [-20, 20])
        c_roi = grid_c[(grid_c['x'] == 0) & (grid_c['y'] >= -20) & (grid_c['y'] <= 20)]
        u_c = c_roi['roiCounts'].mean()

        # Calculate CNR and relative drop
        cnr = abs(u_c - u_norm) / std_norm if std_norm > 0 else np.nan
        rel_drop = (u_norm - u_c) / u_norm * 100.0 if u_norm > 0 else np.nan

        results.append({
            'File': os.path.basename(file_path),
            'Crack_Size_cm': size_val,
            'u_crack': round(u_c, 2),
            'u_normal': round(u_norm, 2),
            'Drop_%': round(rel_drop, 2),
            'CNR': round(cnr, 2)
        })

        # Line profile slice at Y = 0 cm
        c_line = grid_c[grid_c['y'] == 0].sort_values(by='x')
        crack_profiles[size_val] = c_line[(c_line['x'] >= -5) & (c_line['x'] <= 5)]

    df_summary = pd.DataFrame(results).sort_values(by='Crack_Size_cm')
    cnr_map = df_summary.set_index('Crack_Size_cm')['CNR'].to_dict()

    # 3. Simpan tabel ringkasan ke file CSV
    summary_csv_path = os.path.join(output_dir, 'batch_cnr_summary.csv')
    df_summary.to_csv(summary_csv_path, index=False)
    print(f"[INFO] Summary table saved to: {summary_csv_path}")

    # 4. Ekspor data profil garis untuk makro ROOT C
    line_profiles_csv = os.path.join(output_dir, 'line_profiles_data.csv')
    export_line_profiles_csv(norm_line_sub, crack_profiles, cnr_map, line_profiles_csv)

    print("\n=== SUMMARY TABLE (ALL CRACKS) ===")
    print(df_summary.to_string(index=False))

    # 5. Jalankan kedua makro ROOT C untuk menghasilkan seluruh grafik
    print("\n[INFO] Menjalankan makro ROOT C: line_profiles_plot.C ...")
    os.system("root -l -b -q line_profiles_plot.C")

    print("[INFO] Menjalankan makro ROOT C: cnr_sensitivity_plot.C ...")
    os.system("root -l -b -q cnr_sensitivity_plot.C")

    # Bersihkan artefak sementara non-standar jika ada
    legacy_files = [
        os.path.join(output_dir, 'analisis_multi_crack_paper.png'),
        os.path.join(output_dir, 'cnr_detection_sensitivity_root.png')
    ]
    for lf in legacy_files:
        if os.path.exists(lf):
            try:
                os.remove(lf)
            except OSError:
                pass

    print(f"\n[DONE] Seluruh grafik dan tabel di folder '{output_dir}/' 100% dibuat dengan ROOT C!")

if __name__ == '__main__':
    main()

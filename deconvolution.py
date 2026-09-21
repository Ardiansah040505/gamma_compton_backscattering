import os
import glob
import numpy as np
import pandas as pd

try:
    import scipy.signal as signal
except ImportError:
    signal = None

# -----------------------------------------------------------------------------
# METODE 1: Wiener Deconvolution (Restorasi Sinyal PSF)
# -----------------------------------------------------------------------------
def apply_wiener_deconv(profile_1d, psf_kernel, noise_power=0.01):
    """
    Mengeliminasi efek pelebaran (blurring) PSF detektor di domain frekuensi.
    Jika psf_kernel memiliki puncak di tengah, dilakukan circular shift (ifftshift)
    agar fase kernel selaras pada frekuensi nol (tanpa pergeseran posisi).
    """
    if np.argmax(psf_kernel) != 0:
        kernel = np.fft.ifftshift(psf_kernel)
    else:
        kernel = psf_kernel

    I_fft = np.fft.fft(profile_1d)
    H_fft = np.fft.fft(kernel, n=len(profile_1d))

    # Filter Wiener
    H_conj = np.conj(H_fft)
    wiener_filter = H_conj / (np.abs(H_fft)**2 + noise_power)

    # Rekonstruksi sinyal murni
    profile_deconv = np.real(np.fft.ifft(I_fft * wiener_filter))
    return profile_deconv

# -----------------------------------------------------------------------------
# METODE 2: Turunan Pertama (dI/dx Peak-to-Peak Distance)
# -----------------------------------------------------------------------------
def measure_width_derivative(x_positions, profile_1d):
    """
    Mendeteksi koordinat fisik dinding retakan berdasarkan puncak gradien turunan.
    """
    dx = x_positions[1] - x_positions[0]
    gradient = np.gradient(profile_1d, dx)

    # Puncak positif (dinding retakan kiri) dan puncak negatif (dinding kanan)
    idx_left = np.argmax(gradient)
    idx_right = np.argmin(gradient)

    return np.abs(x_positions[idx_right] - x_positions[idx_left])

# -----------------------------------------------------------------------------
# METODE 3: Kurva Kalibrasi Polinomial Derajat 2
# -----------------------------------------------------------------------------
def calibrate_fwhm_polynomial(fwhm_raw_array, crack_true_array):
    """
    Membuat persamaan koreksi non-linear: d_true = a*(FWHM)^2 + b*(FWHM) + c
    """
    coeffs = np.polyfit(fwhm_raw_array, crack_true_array, deg=2)
    return coeffs

# -----------------------------------------------------------------------------
# KALKULASI FWHM (Full Width at Half Maximum)
# -----------------------------------------------------------------------------
def calculate_fwhm(x, profile):
    """
    Menghitung Full Width at Half Maximum (FWHM) dari profil 1D
    menggunakan interpolasi linear pada titik potong Half-Maximum.
    Profil diasumsikan memiliki puncak positif dominan.
    """
    p = np.array(profile, dtype=float)
    x = np.array(x, dtype=float)

    p_max_idx = np.argmax(p)
    p_max = p[p_max_idx]
    x_peak = x[p_max_idx]

    # Baseline diambil dari rata-rata kedua ujung profil
    p_baseline = (p[0] + p[-1]) / 2.0
    delta_p = p_max - p_baseline
    if delta_p <= 0:
        return np.nan, np.nan, np.nan, x_peak

    p_half = p_baseline + delta_p / 2.0

    # Titik potong sisi kiri (crossing p_half)
    x_left = np.nan
    for i in range(p_max_idx):
        if (p[i] <= p_half and p[i+1] >= p_half) or (p[i] >= p_half and p[i+1] <= p_half):
            x1, x2 = x[i], x[i+1]
            y1, y2 = p[i], p[i+1]
            if y2 != y1:
                x_left = x1 + (p_half - y1) * (x2 - x1) / (y2 - y1)
                break

    # Titik potong sisi kanan (crossing p_half)
    x_right = np.nan
    for i in range(p_max_idx, len(x) - 1):
        if (p[i] >= p_half and p[i+1] <= p_half) or (p[i] <= p_half and p[i+1] >= p_half):
            x1, x2 = x[i], x[i+1]
            y1, y2 = p[i], p[i+1]
            if y2 != y1:
                x_right = x1 + (p_half - y1) * (x2 - x1) / (y2 - y1)
                break

    fwhm = x_right - x_left
    return fwhm, x_left, x_right, x_peak

# -----------------------------------------------------------------------------
# PARSING GROUND TRUTH / LEBAR SEBENARNYA
# -----------------------------------------------------------------------------
def parse_actual(filename):
    """
    Mendapatkan lebar retakan sebenarnya (ground truth) dalam cm dari nama file.
    """
    clean_name = filename.replace("crack_normal_combined_relative_diff_", "").replace(".csv", "")
    if clean_name == 'crack1':
        return 1.0
    if clean_name == 'crack2cm':
        return 2.0
    s = clean_name.replace('crack', '').replace('cm', '').replace('_', '.')
    try:
        return float(s)
    except ValueError:
        return np.nan

# -----------------------------------------------------------------------------
# FUNGSI UTAMA (MAIN EXECUTION)
# -----------------------------------------------------------------------------
def main():
    import argparse
    parser = argparse.ArgumentParser(description="Analisis Dekonvolusi Wiener & FWHM Keretakan Pipa GCB")
    parser.add_argument("--sigma", type=float, default=0.50, help="Lebar PSF Gaussian detektor (cm), default=0.50")
    parser.add_argument("--noise-power", type=float, default=0.01, help="Rasio derau regulasi Wiener, default=0.01")
    parser.add_argument("--data-dir", type=str, default=None, help="Direktori file CSV relative difference")
    args = parser.parse_args()

    # Deteksi lokasi file CSV secara cerdas
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidate_dirs = [
        args.data_dir,
        script_dir,
        os.path.join(script_dir, "hasil_sementara"),
        os.getcwd(),
        os.path.join(os.getcwd(), "hasil_sementara")
    ]
    
    files = []
    base_dir = script_dir
    for cand in candidate_dirs:
        if cand and os.path.isdir(cand):
            matched = glob.glob(os.path.join(cand, "crack_normal_combined_relative_diff_*.csv"))
            if matched:
                files = matched
                base_dir = cand
                break
        
    if not files:
        print(f"Error: Tidak ditemukan file 'crack_normal_combined_relative_diff_*.csv' di candidate directories.")
        return

    # Urutkan file berdasarkan lebar sebenarnya (Ground Truth)
    file_list = []
    for f in files:
        fname = os.path.basename(f)
        actual_w = parse_actual(fname)
        file_list.append((f, fname, actual_w))
    file_list.sort(key=lambda item: item[2] if not np.isnan(item[2]) else 999)

    sigma_psf = args.sigma
    noise_power = args.noise_power

    print("=" * 105)
    print(" ANALISIS DEKONVOLUSI WIENER & PENGUKURAN FWHM KERETAKAN ".center(105, "="))
    print("=" * 105)
    print(f"Direktori Data           : {base_dir}")
    print(f"Jumlah Dataset Ditemukan : {len(file_list)}")
    print(f"Parameter Model PSF      : Gaussian (sigma = {sigma_psf:.2f} cm)")
    print(f"Regularisasi Noise Power : {noise_power:.4f}")
    print("=" * 105)

    results = []

    for file_path, file_name, actual_w in file_list:
        clean_tag = file_name.replace("crack_normal_combined_relative_diff_", "").replace(".csv", "")
        df = pd.read_csv(file_path)

        # Region of Interest (ROI) pada posisi retakan
        # X dalam rentang [-5, 5] cm melintang retakan, Y dalam rentang [15, 30] cm
        zoom = df[(df['x'] >= -5) & (df['x'] <= 5) & (df['y'] >= 15) & (df['y'] <= 30)]
        if zoom.empty:
            continue

        # Ekstraksi Line Profile ROI Counts (Relative Difference)
        p_roi = zoom.groupby('x')['roiCounts_rel_diff'].mean().reset_index()
        x_coords = p_roi['x'].values
        # Invert: Penurunan cacah di retakan diubah menjadi puncak positif
        y_raw = -p_roi['roiCounts_rel_diff'].values

        # 1. Hitung FWHM Awal (Sebelum Dekonvolusi)
        fwhm_raw, x_left_raw, x_right_raw, peak_x_raw = calculate_fwhm(x_coords, y_raw)

        # 2. Pembuatan Kernel PSF Gaussian pada koordinat x yang sama
        psf_kernel = np.exp(-x_coords**2 / (2.0 * sigma_psf**2))
        psf_kernel /= psf_kernel.sum()

        # 3. Lakukan Wiener Deconvolution
        y_deconv = apply_wiener_deconv(y_raw, psf_kernel, noise_power=noise_power)

        # 4. Hitung FWHM Hasil Dekonvolusi
        fwhm_dec, x_left_dec, x_right_dec, peak_x_dec = calculate_fwhm(x_coords, y_deconv)

        # 5. Hitung Metode Turunan Pertama (Metode 2) pada grid halus
        x_fine = np.linspace(x_coords[0], x_coords[-1], 201)
        y_raw_fine = np.interp(x_fine, x_coords, y_raw)
        y_dec_fine = np.interp(x_fine, x_coords, y_deconv)
        w_deriv_raw = measure_width_derivative(x_fine, y_raw_fine)
        w_deriv_dec = measure_width_derivative(x_fine, y_dec_fine)

        # Hitung Error terhadap Lebar Sebenarnya
        err_raw = fwhm_raw - actual_w
        rel_err_raw = (err_raw / actual_w) * 100.0 if actual_w > 0 else np.nan

        err_dec = fwhm_dec - actual_w
        rel_err_dec = (err_dec / actual_w) * 100.0 if actual_w > 0 else np.nan

        results.append({
            'Tag': clean_tag,
            'Actual_cm': actual_w,
            'FWHM_Raw_cm': fwhm_raw,
            'FWHM_Dec_cm': fwhm_dec,
            'Err_Raw_cm': err_raw,
            'Err_Dec_cm': err_dec,
            'RelErr_Raw_pct': rel_err_raw,
            'RelErr_Dec_pct': rel_err_dec,
            'Deriv_Raw_cm': w_deriv_raw,
            'Deriv_Dec_cm': w_deriv_dec
        })

    res_df = pd.DataFrame(results)

    # -------------------------------------------------------------------------
    # CETAK TABEL PERBANDINGAN FWHM RAW VS DEKONVOLUSI KE TERMINAL
    # -------------------------------------------------------------------------
    print(f"{'No':<4} {'Retakan':<12} {'Asli (cm)':<10} {'FWHM Raw':<11} {'FWHM Dec':<11} {'Err Raw':<10} {'Err Dec':<10} {'%Err Raw':<10} {'%Err Dec':<10}")
    print("-" * 105)
    for idx, row in res_df.iterrows():
        sign_raw = f"{row['Err_Raw_cm']:+.3f}"
        sign_dec = f"{row['Err_Dec_cm']:+.3f}"
        print(f"{idx+1:<4} {row['Tag']:<12} {row['Actual_cm']:<10.2f} {row['FWHM_Raw_cm']:<11.3f} {row['FWHM_Dec_cm']:<11.3f} {sign_raw:<10} {sign_dec:<10} {row['RelErr_Raw_pct']:<+10.1f} {row['RelErr_Dec_pct']:<+10.1f}")
    print("=" * 105)

    # -------------------------------------------------------------------------
    # STATISTIK RINGKASAN PERFORMA
    # -------------------------------------------------------------------------
    valid_df = res_df.dropna(subset=['Actual_cm', 'FWHM_Raw_cm', 'FWHM_Dec_cm'])
    
    mae_raw = np.mean(np.abs(valid_df['Err_Raw_cm']))
    mae_dec = np.mean(np.abs(valid_df['Err_Dec_cm']))
    rmse_raw = np.sqrt(np.mean(valid_df['Err_Raw_cm']**2))
    rmse_dec = np.sqrt(np.mean(valid_df['Err_Dec_cm']**2))

    # Analisis spesifik untuk retakan kecil (<= 1.0 cm) di mana efek PSF paling dominan
    small_df = valid_df[valid_df['Actual_cm'] <= 1.0]
    mae_raw_small = np.mean(np.abs(small_df['Err_Raw_cm']))
    mae_dec_small = np.mean(np.abs(small_df['Err_Dec_cm']))

    print("\n" + "=" * 70)
    print(" RINGKASAN AKURASI & EFEKTIVITAS DEKONVOLUSI ".center(70, "="))
    print("=" * 70)
    print(f"1. RETAKAN KECIL (<= 1.0 cm, Area Dominasi Blurring PSF):")
    print(f"   * MAE FWHM Raw               : {mae_raw_small:.4f} cm")
    print(f"   * MAE FWHM Dekonvolusi       : {mae_dec_small:.4f} cm")
    print(f"   * Peningkatan Akurasi        : {((mae_raw_small - mae_dec_small) / mae_raw_small) * 100.0:+.2f}%")
    print(f"\n2. KESELURUHAN DATASET (0.1 cm s/d 2.5 cm):")
    print(f"   * Mean Absolute Error (Raw)  : {mae_raw:.4f} cm")
    print(f"   * Mean Absolute Error (Dec)  : {mae_dec:.4f} cm")
    print(f"   * RMSE Raw vs Dekonvolusi    : {rmse_raw:.4f} cm vs {rmse_dec:.4f} cm")
    print("=" * 70)

    # -------------------------------------------------------------------------
    # METODE 3: KALIBRASI POLINOMIAL DERAJAT 2
    # -------------------------------------------------------------------------
    raw_array = valid_df['FWHM_Raw_cm'].values
    dec_array = valid_df['FWHM_Dec_cm'].values
    true_array = valid_df['Actual_cm'].values

    coeffs_raw = calibrate_fwhm_polynomial(raw_array, true_array)
    coeffs_dec = calibrate_fwhm_polynomial(dec_array, true_array)

    pred_cal_raw = np.polyval(coeffs_raw, raw_array)
    pred_cal_dec = np.polyval(coeffs_dec, dec_array)

    mae_cal_raw = np.mean(np.abs(pred_cal_raw - true_array))
    mae_cal_dec = np.mean(np.abs(pred_cal_dec - true_array))

    print("\n" + "=" * 80)
    print(" METODE 3: MODEL KALIBRASI POLINOMIAL (d_true = a*W^2 + b*W + c) ".center(80, "="))
    print("=" * 80)
    print(f"Persamaan Kalibrasi FWHM Raw        : d = ({coeffs_raw[0]:.4f})*W^2 + ({coeffs_raw[1]:.4f})*W + ({coeffs_raw[2]:.4f})")
    print(f"MAE Setelah Kalibrasi FWHM Raw      : {mae_cal_raw:.4f} cm")
    print(f"Persamaan Kalibrasi FWHM Dekonvolusi: d = ({coeffs_dec[0]:.4f})*W^2 + ({coeffs_dec[1]:.4f})*W + ({coeffs_dec[2]:.4f})")
    print(f"MAE Setelah Kalibrasi Dekonvolusi   : {mae_cal_dec:.4f} cm")
    print("=" * 80)

    # Simpan hasil ke CSV
    out_csv = os.path.join(base_dir, "deconvolution_fwhm_comparison.csv")
    res_df['Calibrated_Raw_cm'] = pred_cal_raw
    res_df['Calibrated_Dec_cm'] = pred_cal_dec
    res_df.to_csv(out_csv, index=False)
    print(f"\n[OK] Hasil perbandingan lengkap berhasil disimpan ke: {out_csv}\n")

if __name__ == "__main__":
    main()
import numpy as np
import pandas as pd

# 1. Masukkan dimensi aktual (Ground Truth) dan hasil pengukuran dari profil/citra
# Ganti nilai d_measured dengan hasil pembacaan FWHM atau deteksi tepi data Anda
data_evaluasi = {
    'Crack_Sample': ['Crack 0.1cm', 'Crack 0.3cm', 'Crack 0.5cm', 'Crack 1.0cm', 'Crack 1.5cm', 'Crack 2.0cm', 'Crack 2.5cm'],
    'd_true_cm':    np.array([0.10, 0.30, 0.50, 1.00, 1.50, 2.00, 2.50]),
    'd_measured_cm': np.array([0.20, 0.38, 0.56, 1.07, 1.55, 2.04, 2.52])  # contoh estimasi terukur
}

df = pd.DataFrame(data_evaluasi)

# 2. Hitung selisih absolut dan error relatif per sampel
df['Abs_Error_cm'] = np.abs(df['d_measured_cm'] - df['d_true_cm'])
df['Pct_Error_%']  = (df['Abs_Error_cm'] / df['d_true_cm']) * 100

# 3. Hitung MAE, RMSE, dan MAPE global
N = len(df)
mae = df['Abs_Error_cm'].mean()
rmse = np.sqrt(np.mean((df['d_measured_cm'] - df['d_true_cm'])**2))
mape = df['Pct_Error_%'].mean()

print("=== TABEL EVALUASI AKURASI DIMENSI RETAKAN ===")
print(df.to_string(index=False))
print("-" * 50)
print(f"MAE  (Mean Absolute Error)     : {mae:.4f} cm")
print(f"RMSE (Root Mean Square Error)  : {rmse:.4f} cm")
print(f"MAPE (Mean Absolute Pct Error) : {mape:.2f} %")
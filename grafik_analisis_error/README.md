# Folder Analisis & Visualisasi Error Gabungan Keretakan GCB (Nilai Absolut)

Folder ini berisi skrip visualisasi, rekapitulasi data, dan gambar grafik terpadu untuk menganalisis error estimasi dimensi keretakan pipa (0.1 cm s/d 2.5 cm) pada sistem *Gamma Compton Backscattering* (GCB).

Seluruh nilai error dihitung dalam bentuk **nilai mutlak/absolut** ($|\text{Error}| \ge 0$), sehingga tidak ada nilai negatif. Nilai $0$ merepresentasikan kondisi ideal (*zero error*).

## Daftar File & Gambar yang Dihasilkan

1. **`generate_error_plots.py`**
   - Skrip Python untuk meregenerasi seluruh grafik dan tabel rekapitulasi error secara otomatis.
   - Cara menjalankan: `python3 generate_error_plots.py`

2. **`grafik_error_dual_panel.png`** (Rekomendasi Utama Publikasi/Skripsi)
   - Grafik 2 panel berdampingan:
     - **Panel (a)**: Error Absolut ($|W_{FWHM} - W_{aktual}|$) dalam satuan cm (skala 0 s/d 0.75 cm).
     - **Panel (b)**: Error Relatif Absolut ($\frac{|W_{FWHM} - W_{aktual}|}{W_{aktual}} \times 100\%$) dalam % (skala 0 s/d 600%).

3. **`grafik_error_dual_axis.png`**
   - Grafik tunggal dengan sumbu Y kembar (*Twin Y-Axis*).
   - Sumbu Y Kiri: Error Absolut $|\Delta W|$ (cm).
   - Sumbu Y Kanan: Error Relatif Absolut (%).
   - Dilengkapi penanda zona *blurring* kolimator/PSF detektor (< 1.0 cm).

4. **`grafik_komparasi_metode_dekonvolusi.png`**
   - Membandingkan error mutlak FWHM Mentah (*Raw*) vs Dekonvolusi Wiener vs Dekonvolusi Richardson-Lucy di seluruh ukuran retakan.

5. **`grafik_reduksi_error_kalibrasi.png`**
   - Memperlihatkan penurunan error mutlak setelah diterapkan model kalibrasi polinomial $d = aW^2 + bW + c$, memangkas MAE dari $0.301\text{ cm}$ menjadi $0.048\text{ cm}$ ($0.045\text{ cm}$ untuk dekonvolusi+kalibrasi).

6. **`rekap_error_keseluruhan.csv`**
   - Tabel komprehensif berisi seluruh nilai ground truth, FWHM hasil ukur, error absolut, error relatif absolut, dan hasil kalibrasi dari 23 variasi ukuran keretakan.

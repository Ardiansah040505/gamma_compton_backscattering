# Folder Analisis & Visualisasi Error Keretakan Horizontal GCB (Nilai Absolut)

Folder ini berisi skrip visualisasi, rekapitulasi data, dan gambar grafik terpadu untuk menganalisis error estimasi dimensi keretakan pipa horizontal (0.3 cm s/d 2.5 cm) pada sistem *Gamma Compton Backscattering* (GCB).

Seluruh nilai error dihitung dalam bentuk **nilai mutlak/absolut** ($|\text{Error}| \ge 0$), sehingga **tidak ada nilai negatif**. Nilai $0$ merepresentasikan kondisi ideal (*zero error*).

---

## Daftar File & Gambar yang Dihasilkan

1. **`generate_error_plots.py`**
   - Skrip Python untuk meregenerasi seluruh grafik dan tabel rekapitulasi error secara otomatis.
   - Cara menjalankan: `python3 generate_error_plots.py`

2. **`grafik_error_dual_panel.png`** (Rekomendasi Utama Publikasi/Skripsi)
   - Grafik 2 panel berdampingan:
     - **Panel (a)**: Error Absolut ($|W_{FWHM} - W_{aktual}|$) dalam satuan cm (skala 0 s/d 0.85 cm).
     - **Panel (b)**: Error Relatif Absolut ($\frac{|W_{FWHM} - W_{aktual}|}{W_{aktual}} \times 100\%$) dalam % (skala 0 s/d 260%).

3. **`grafik_error_dual_axis.png`**
   - Grafik tunggal dengan sumbu Y kembar (*Twin Y-Axis*).
   - Sumbu Y Kiri: Error Absolut $|\Delta W|$ (cm).
   - Sumbu Y Kanan: Error Relatif Absolut (%).
   - Dilengkapi penanda zona *blurring* kolimator/PSF detektor (< 1.0 cm).

4. **`grafik_komparasi_fwhm_vs_aktual.png`**
   - Membandingkan hasil estimasi FWHM dari cacah detektor (`roiCounts`) dan laju fluks (`normalizedFlux`) terhadap garis ideal ($y = x$).

5. **`grafik_reduksi_error_kalibrasi.png`**
   - Memperlihatkan penurunan error mutlak setelah diterapkan model kalibrasi polinomial $d = aW^2 + bW + c$, memangkas MAE dari $0.2472\text{ cm}$ ($2.47\text{ mm}$) menjadi $0.0803\text{ cm}$ ($0.80\text{ mm}$), reduksi error sebesar **67.50%**.

6. **`rekap_error_keseluruhan.csv`**
   - Tabel komprehensif berisi ground truth ($W_{aktual}$), FWHM hasil ukur (`roiCounts` dan `normalizedFlux`), nilai error absolut, error relatif absolut, hasil kalibrasi, serta hasil spatial averaging dari 16 variasi keretakan horizontal.

---

## Ringkasan Hasil Analisis Keretakan Horizontal

| No | Label Retakan | Lebar Aktual ($W_{aktual}$) | FWHM Terukur ($W_{FWHM}$) | Error Absolut ($|\Delta W|$) | Error Relatif Absolut ($|\%|$) | Hasil Kalibrasi | Error Kalibrasi |
|:--:|:-------------:|:---------------------------:|:-------------------------:|:----------------------------:|:------------------------------:|:---------------:|:---------------:|
| 1  | crack0.3cm    | 0.30 cm                     | 1.0280 cm                 | 0.7280 cm                    | 242.66%                        | 0.5984 cm       | 0.2984 cm       |
| 2  | crack0.5cm    | 0.50 cm                     | 0.9631 cm                 | 0.4631 cm                    | 92.62%                         | 0.4332 cm       | 0.0668 cm       |
| 3  | crack0.7cm    | 0.70 cm                     | 1.0484 cm                 | 0.3484 cm                    | 49.77%                         | 0.6489 cm       | 0.0511 cm       |
| 4  | crack0.8cm    | 0.80 cm                     | 1.0892 cm                 | 0.2892 cm                    | 36.15%                         | 0.7481 cm       | 0.0519 cm       |
| 5  | crack0.9cm    | 0.90 cm                     | 1.1047 cm                 | 0.2047 cm                    | 22.75%                         | 0.7850 cm       | 0.1150 cm       |
| 6  | crack1.0cm    | 1.00 cm                     | 1.1793 cm                 | 0.1793 cm                    | 17.93%                         | 0.9571 cm       | 0.0429 cm       |
| 7  | crack1.1cm    | 1.10 cm                     | 1.2911 cm                 | 0.1911 cm                    | 17.37%                         | 1.1985 cm       | 0.0985 cm       |
| 8  | crack1.2cm    | 1.20 cm                     | 1.3138 cm                 | 0.1138 cm                    | 9.49%                          | 1.2453 cm       | 0.0453 cm       |
| 9  | crack1.4cm    | 1.40 cm                     | 1.3804 cm                 | 0.0196 cm                    | 1.40%                          | 1.3770 cm       | 0.0230 cm       |
| 10 | crack1.6cm    | 1.60 cm                     | 1.4256 cm                 | 0.1744 cm                    | 10.90%                         | 1.4624 cm       | 0.1376 cm       |
| 11 | crack1.7cm    | 1.70 cm                     | 1.5144 cm                 | 0.1856 cm                    | 10.92%                         | 1.6206 cm       | 0.0794 cm       |
| 12 | crack1.9cm    | 1.90 cm                     | 1.7203 cm                 | 0.1797 cm                    | 9.46%                          | 1.9383 cm       | 0.0383 cm       |
| 13 | crack2.0cm    | 2.00 cm                     | 1.8410 cm                 | 0.1590 cm                    | 7.95%                          | 2.0928 cm       | 0.0928 cm       |
| 14 | crack2.1cm    | 2.10 cm                     | 1.8821 cm                 | 0.2179 cm                    | 10.37%                         | 2.1401 cm       | 0.0401 cm       |
| 15 | crack2.3cm    | 2.30 cm                     | 2.0796 cm                 | 0.2204 cm                    | 9.58%                          | 2.3292 cm       | 0.0292 cm       |
| 16 | crack2.5cm    | 2.50 cm                     | 2.2195 cm                 | 0.2805 cm                    | 11.22%                         | 2.4251 cm       | 0.0749 cm       |

- **Rata-rata Error Absolut (MAE Raw FWHM)**: **$0.2472\text{ cm}$ ($2.47\text{ mm}$)**
- **Rata-rata Error Absolut Setelah Kalibrasi (MAE Calibrated)**: **$0.0803\text{ cm}$ ($0.80\text{ mm}$)**
- **Persentase Reduksi Error**: **$67.50\%$**

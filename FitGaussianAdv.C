#include <iostream>
#include <iomanip>
#include <sstream>
#include "TFile.h"
#include "TH1D.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"

using namespace std;

// =============================================================================
//  FitGaussianAdv.C
//  ROOT macro untuk Gaussian fitting dengan kontrol parameter energi manual.
//
//  CARA PAKAI (di ROOT prompt atau .x):
//
//    // Mode AUTO: parameter awal diestimasi otomatis dari histogram
//    FitGaussianAdv(600.0, 720.0);
//
//    // Mode MANUAL: tentukan sendiri amplitudo, mean, dan sigma awal
//    // Berguna saat fitting kurang akurat atau peak tidak di tengah range
//    FitGaussianAdv(600.0, 720.0,          // range energi (keV)
//                   "h_scan_5090",          // nama histogram
//                   "spectrum_det0_5090_5092.root", // file ROOT
//                   2500.0,                 // amp_init  (tinggi puncak, counts)
//                   661.0,                  // mean_init (posisi puncak, keV)
//                   15.0,                   // sigma_init (lebar, keV)
//                   true,                   // simpan canvas ke PNG
//                   true);                  // simpan canvas ke PDF
//
//  Catatan:
//    - Set amp_init  = 0 untuk estimasi otomatis
//    - Set mean_init = 0 untuk estimasi otomatis (tengah range)
//    - Set sigma_init = 0 untuk estimasi otomatis ((E_max-E_min)/6)
// =============================================================================

/**
 * Estimasi parameter awal Gaussian dari region histogram.
 *
 * @param h          Pointer histogram
 * @param E_min      Batas bawah range (keV)
 * @param E_max      Batas atas range (keV)
 * @param amp        [in/out] Jika > 0, dipakai apa adanya; jika 0, di-estimasi
 * @param mean       [in/out] Jika > 0, dipakai apa adanya; jika 0, di-estimasi
 * @param sigma      [in/out] Jika > 0, dipakai apa adanya; jika 0, di-estimasi
 */
void AutoInitParam(TH1D* h, double E_min, double E_max,
                   double& amp, double& mean, double& sigma)
{
    int bin_min = h->FindBin(E_min);
    int bin_max = h->FindBin(E_max);

    // Amplitudo = nilai bin tertinggi dalam range
    double amp_auto = 0.0;
    int    peak_bin = bin_min;
    for (int b = bin_min; b <= bin_max; ++b) {
        if (h->GetBinContent(b) > amp_auto) {
            amp_auto = h->GetBinContent(b);
            peak_bin = b;
        }
    }

    // Mean = posisi bin tertinggi; fallback ke tengah range
    double mean_auto  = (amp_auto > 0) ? h->GetBinCenter(peak_bin)
                                       : (E_min + E_max) / 2.0;
    // Sigma = 1/6 lebar range (aturan 3-sigma mencakup ~99%)
    double sigma_auto = (E_max - E_min) / 6.0;

    if (amp   <= 0) amp   = amp_auto;
    if (mean  <= 0) mean  = mean_auto;
    if (sigma <= 0) sigma = sigma_auto;
}

// =============================================================================
//  Fungsi utama
// =============================================================================

/**
 * Lakukan Gaussian fitting pada satu peak dalam histogram spektrum gamma.
 *
 * @param E_min       Batas bawah energi untuk fitting (keV)  [wajib]
 * @param E_max       Batas atas energi untuk fitting (keV)   [wajib]
 * @param histName    Nama histogram di dalam file ROOT
 * @param fileName    Path file ROOT
 * @param amp_init    Parameter awal amplitudo  (0 = auto)
 * @param mean_init   Parameter awal mean/posisi puncak (0 = auto)
 * @param sigma_init  Parameter awal sigma  (0 = auto)
 * @param savePNG     Simpan canvas ke file PNG jika true
 * @param savePDF     Simpan canvas ke file PDF jika true
 */
void FitGaussianAdv(double      E_min      = 600.0,
                    double      E_max      = 720.0,
                    const char* histName   = "h_scan_5090",
                    const char* fileName   = "spectrum_det0_5090_5092.root",
                    double      amp_init   = 0.0,
                    double      mean_init  = 0.0,
                    double      sigma_init = 0.0,
                    bool        savePNG    = true,
                    bool        savePDF    = true)
{
    // ------------------------------------------------------------------
    // 1. Validasi input
    // ------------------------------------------------------------------
    if (E_min >= E_max) {
        cout << "[ERROR] E_min (" << E_min << ") harus < E_max (" << E_max << ")!" << endl;
        return;
    }

    // ------------------------------------------------------------------
    // 2. Buka file ROOT
    // ------------------------------------------------------------------
    TFile* f = TFile::Open(fileName, "READ");
    if (!f || f->IsZombie()) {
        cout << "[ERROR] Gagal membuka file: " << fileName << endl;
        return;
    }

    // ------------------------------------------------------------------
    // 3. Ambil histogram
    // ------------------------------------------------------------------
    TH1D* h = (TH1D*)f->Get(histName);
    if (!h) {
        cout << "[ERROR] Histogram '" << histName << "' tidak ditemukan dalam "
             << fileName << "!" << endl;
        f->Close();
        return;
    }
    h->SetDirectory(0);   // detach dari file agar tetap hidup setelah Close
    f->Close();

    // ------------------------------------------------------------------
    // 4. Aktifkan Sumw2 untuk error bar yang benar
    // ------------------------------------------------------------------
    if (h->GetSumw2N() == 0) h->Sumw2();

    // ------------------------------------------------------------------
    // 5. Estimasi / validasi parameter awal
    // ------------------------------------------------------------------
    AutoInitParam(h, E_min, E_max, amp_init, mean_init, sigma_init);

    // Sigma tidak boleh <= 0
    if (sigma_init <= 0) sigma_init = (E_max - E_min) / 6.0;

    // ------------------------------------------------------------------
    // 6. Buat dan konfigurasi fungsi Gaussian
    // ------------------------------------------------------------------
    TF1* gausFit = new TF1("gausFit", "gaus", E_min, E_max);
    gausFit->SetLineColor(kRed + 1);
    gausFit->SetLineWidth(2);
    gausFit->SetParameters(amp_init, mean_init, sigma_init);
    gausFit->SetParNames("Constant", "Mean", "Sigma");

    // Batas bawah Sigma agar tidak negatif
    gausFit->SetParLimits(2, 0.0, E_max - E_min);

    // ------------------------------------------------------------------
    // 7. Cetak parameter awal ke terminal
    // ------------------------------------------------------------------
    cout << "\n============= PARAMETER AWAL FITTING ===========================" << endl;
    cout << fixed << setprecision(3);
    cout << "  Range Fit    : [" << E_min << ", " << E_max << "] keV" << endl;
    cout << "  amp_init     : " << amp_init   << " (counts)" << endl;
    cout << "  mean_init    : " << mean_init  << " keV" << endl;
    cout << "  sigma_init   : " << sigma_init << " keV" << endl;
    cout << "================================================================\n" << endl;

    // ------------------------------------------------------------------
    // 8. Lakukan fitting
    //    "R" = gunakan range TF1
    //    "S" = simpan TFitResultPtr
    //    "Q" = quiet (tidak cetak ulang oleh ROOT)
    //    "M" = mencari minimum yang lebih baik (IMPROVE)
    // ------------------------------------------------------------------
    gStyle->SetOptFit(1111);
    auto fitResult = h->Fit(gausFit, "R S Q M");

    // ------------------------------------------------------------------
    // 9. Ekstrak hasil fitting
    // ------------------------------------------------------------------
    double constant     = gausFit->GetParameter(0);
    double constant_err = gausFit->GetParError(0);
    double mean         = gausFit->GetParameter(1);
    double mean_err     = gausFit->GetParError(1);
    double sigma        = gausFit->GetParameter(2);
    double sigma_err    = gausFit->GetParError(2);

    double fwhm         = 2.35482 * sigma;
    double fwhm_err     = 2.35482 * sigma_err;
    double resolution   = (mean > 0) ? (fwhm / mean) * 100.0 : 0.0;

    double chi2         = gausFit->GetChisquare();
    int    ndf          = gausFit->GetNDF();
    double chi2ndf      = (ndf > 0) ? chi2 / ndf : 0.0;

    bool   converged    = fitResult.Get() && fitResult->IsValid();

    // ------------------------------------------------------------------
    // 10. Cetak ringkasan hasil ke terminal
    // ------------------------------------------------------------------
    cout << "\n=================== HASIL FITTING GAUSSIAN ===================" << endl;
    cout << "  File         : " << fileName  << endl;
    cout << "  Histogram    : " << histName  << endl;
    cout << "  Range Fit    : [" << E_min << ", " << E_max << "] keV" << endl;
    cout << "--------------------------------------------------------------" << endl;
    cout << fixed << setprecision(4);
    cout << "  Constant     : " << constant << " +/- " << constant_err << " counts" << endl;
    cout << "  Mean (mu)    : " << mean     << " +/- " << mean_err     << " keV"    << endl;
    cout << "  Sigma        : " << sigma    << " +/- " << sigma_err    << " keV"    << endl;
    cout << "  FWHM         : " << fwhm     << " +/- " << fwhm_err     << " keV"    << endl;
    cout << fixed << setprecision(3);
    cout << "  Resolusi (R) : " << resolution << " %  (FWHM/Mean x 100)" << endl;
    cout << "  Chi2 / NDF   : " << chi2 << " / " << ndf
         << "  (" << chi2ndf << ")" << endl;
    cout << "  Status Fit   : " << (converged ? "CONVERGED / SUCCESS" : "FAILED") << endl;
    cout << "==============================================================\n" << endl;

    // ------------------------------------------------------------------
    // 11. Visualisasi canvas
    // ------------------------------------------------------------------
    TCanvas* c = new TCanvas("c_fitAdv", "Gaussian Fit - Advanced", 1000, 680);
    c->SetGrid();
    c->SetLeftMargin(0.12);
    c->SetRightMargin(0.05);
    c->SetBottomMargin(0.12);

    // Gaya histogram: garis histogram tanpa error bar
    h->SetLineColor(kBlack);
    h->SetLineWidth(1);
    h->SetFillColorAlpha(kBlack, 0.08);  // isi tipis agar peak terlihat
    h->SetTitle(Form("Gaussian Fit  |  %s  |  %s", histName, fileName));
    h->GetXaxis()->SetTitle("Energi (keV)");
    h->GetYaxis()->SetTitle("Counts");

    h->Draw("HIST");
    gausFit->Draw("SAME");

    c->Update();

    // ------------------------------------------------------------------
    // 12. Simpan canvas ke file (nama otomatis dari histogram + range)
    // ------------------------------------------------------------------
    ostringstream base;
    base << histName << "_fit_"
         << static_cast<int>(E_min) << "_"
         << static_cast<int>(E_max);

    if (savePNG) {
        string outPNG = base.str() + ".png";
        c->SaveAs(outPNG.c_str());
        cout << "[INFO] Canvas disimpan ke: " << outPNG << endl;
    }
    if (savePDF) {
        string outPDF = base.str() + ".pdf";
        c->SaveAs(outPDF.c_str());
        cout << "[INFO] Canvas disimpan ke: " << outPDF << endl;
    }
}

#include <iostream>
#include <iomanip>
#include "TFile.h"
#include "TH1D.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"

using namespace std;

/**
 * Macro ROOT untuk fitting Gaussian pada histogram spektrum gamma.
 * 
 * Parameter:
 *   E_min     : Batas bawah energi untuk fitting (keV)
 *   E_max     : Batas atas energi untuk fitting (keV)
 *   histName  : Nama histogram di dalam file (default: "h_scan_1000")
 *   fileName  : Nama file ROOT target (default: "spectrum_det0_511_1021.root")
 */
void FitGaussian(double E_min = 600.0, 
                 double E_max = 720.0, 
                 const char* histName = "h_scan_5090", 
                 const char* fileName = "spectrum_det0_5090_5092.root") 
{
    // 1. Validasi input rentang energi
    if (E_min >= E_max) {
        cout << "[ERROR] E_min (" << E_min << ") harus lebih kecil dari E_max (" << E_max << ")!" << endl;
        return;
    }

    // 2. Buka file ROOT
    TFile *f = TFile::Open(fileName, "READ");
    if (!f || f->IsZombie()) {
        cout << "[ERROR] Gagal membuka file: " << fileName << endl;
        return;
    }

    // 3. Ambil histogram
    TH1D *h = (TH1D*)f->Get(histName);
    if (!h) {
        cout << "[ERROR] Histogram '" << histName << "' tidak ditemukan dalam " << fileName << "!" << endl;
        return;
    }

    // 4. Pastikan Sumw2 aktif untuk penanganan error bar yang benar
    if (h->GetSumw2N() == 0) {
        h->Sumw2();
    }

    // Tampilkan statistik fitting di box info canvas
    gStyle->SetOptFit(1111);

    // 5. Setup gaya visual histogram (titik data dengan error bar)
    h->SetMarkerStyle(20);
    h->SetMarkerSize(0.7);
    h->SetMarkerColor(kBlack);
    h->SetLineColor(kBlack);

    // 6. Buat fungsi Gaussian dengan rentang [E_min, E_max]
    TF1 *gausFit = new TF1("gausFit", "gaus", E_min, E_max);
    gausFit->SetLineColor(kRed);
    gausFit->SetLineWidth(2);

    // Estimasi parameter awal secara otomatis dari region [E_min, E_max]
    int bin_min = h->FindBin(E_min);
    int bin_max = h->FindBin(E_max);
    double amp_init = 0.0;
    for (int b = bin_min; b <= bin_max; ++b) {
        if (h->GetBinContent(b) > amp_init) amp_init = h->GetBinContent(b);
    }
    double mean_init = (E_min + E_max) / 2.0;
    double sigma_init = (E_max - E_min) / 6.0;

    gausFit->SetParameters(amp_init, mean_init, sigma_init);
    gausFit->SetParNames("Constant", "Mean (Peak)", "Sigma");

    // 7. Lakukan proses fitting
    // "R" = gunakan rentang TF1, "S" = simpan hasil fit (TFitResultPtr), "Q" = quiet di prompt bawaan ROOT
    auto fitResult = h->Fit(gausFit, "R S");

    // 8. Hitung FWHM dan Resolusi Energi (R = FWHM / E * 100%)
    double mean = gausFit->GetParameter(1);
    double mean_err = gausFit->GetParError(1);
    double sigma = gausFit->GetParameter(2);
    double sigma_err = gausFit->GetParError(2);
    double fwhm = 2.35482 * sigma;
    double fwhm_err = 2.35482 * sigma_err;
    double resolution = (mean > 0) ? (fwhm / mean) * 100.0 : 0.0;

    // 9. Cetak ringkasan hasil fitting ke terminal
    cout << "\n=================== HASIL FITTING GAUSSIAN ===================" << endl;
    cout << " File Target       : " << fileName << endl;
    cout << " Histogram         : " << histName << endl;
    cout << " Range Fit [a, b]  : [" << E_min << ", " << E_max << "] keV" << endl;
    cout << "--------------------------------------------------------------" << endl;
    cout << fixed << setprecision(3);
    cout << " Constant (Tinggi) : " << gausFit->GetParameter(0) << " +/- " << gausFit->GetParError(0) << endl;
    cout << " Mean (Posisi Puncak): " << mean << " +/- " << mean_err << " keV" << endl;
    cout << " Sigma             : " << sigma << " +/- " << sigma_err << " keV" << endl;
    cout << " FWHM              : " << fwhm << " +/- " << fwhm_err << " keV" << endl;
    cout << " Resolusi Energi   : " << resolution << " %" << endl;
    cout << " Chi2 / NDF        : " << gausFit->GetChisquare() << " / " << gausFit->GetNDF() 
         << " (" << (gausFit->GetNDF() > 0 ? gausFit->GetChisquare() / gausFit->GetNDF() : 0) << ")" << endl;
    cout << " Status Fit        : " << (fitResult->IsValid() ? "CONVERGED / SUCCESS" : "FAILED") << endl;
    cout << "==============================================================\n" << endl;

    // 10. Tampilkan ke Canvas
    TCanvas *c = new TCanvas("c_fit", "Gaussian Fit Spectrum", 900, 650);
    c->SetGrid();
    
    // Gambar histogram dengan titik data & error bar (SUMW2)
    h->Draw("E1");
    gausFit->Draw("SAME");

    // Tambahkan Legend
    TLegend *leg = new TLegend(0.65, 0.70, 0.88, 0.88);
    leg->AddEntry(h, "Data (Error Bar)", "lep");
    leg->AddEntry(gausFit, "Gaussian Fit", "l");
    leg->Draw();
}

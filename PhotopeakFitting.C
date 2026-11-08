#include <iostream>
#include <iomanip>
#include <cmath>
#include "TFile.h"
#include "TH1D.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TRandom3.h"

using namespace std;

/**
 * Terapkan smearing resolusi energi pada histogram spektrum gamma
 * dengan pemotongan rentang energi (cut energy range / threshold).
 *
 * @param fileName    Path file ROOT (default: "spectrum_det0_5090_5092.root")
 * @param histName    Nama histogram di dalam file ROOT (default: "h_scan_5090")
 * @param resolution  Resolusi energi relatif R = FWHM/E, misalnya 0.07 untuk 7%
 * @param E_ref       Energi referensi untuk skala resolusi (keV), default 662 keV
 * @param cut_min     Batas bawah pemotongan energi (keV), default 600.0 keV
 * @param cut_max     Batas atas pemotongan energi (keV), default 730.0 keV
 * @param outFile     Nama file PNG output
 */
void PhotopeakFitting(const char* fileName   = "spectrum_det0_5090_5092.root",
                   const char* histName   = "h_scan_5090",
                   double      resolution = 0.07,
                   double      E_ref      = 662.0,
                   double      cut_min    = 600.0,
                   double      cut_max    = 730.0,
                   const char* outFile    = "Photopeak_Smearing_Result_Detector_0.png")
{
    // ------------------------------------------------------------------
    // 1. Buka file ROOT dan ambil histogram
    // ------------------------------------------------------------------
    TFile* f = TFile::Open(fileName, "READ");
    if (!f || f->IsZombie()) {
        cout << "[ERROR] Gagal membuka file: " << fileName << endl;
        return;
    }

    TH1D* h_raw = (TH1D*)f->Get(histName);
    if (!h_raw) {
        cout << "[ERROR] Histogram '" << histName << "' tidak ditemukan dalam " 
             << fileName << "!" << endl;
        f->Close();
        return;
    }
    h_raw->SetDirectory(0);   // detach agar tetap hidup setelah file ditutup
    f->Close();

    // ------------------------------------------------------------------
    // 2. Buat histogram baru untuk hasil smearing
    // ------------------------------------------------------------------
    TH1D* h_smeared = (TH1D*)h_raw->Clone("h_smeared");
    h_smeared->Reset();
    h_smeared->Sumw2();
    h_smeared->SetTitle("Spectrum Photopeak 662 Detector 0");
    h_smeared->SetLineColor(kRed);
    h_smeared->SetLineWidth(2);

    // ------------------------------------------------------------------
    // 3. Parameter resolusi — sigma skala sebagai sqrt(E/E_ref)
    // ------------------------------------------------------------------
    double fwhm_ref  = resolution * E_ref;
    double sigma_ref = fwhm_ref / 2.3548;

    TRandom3 rng(0);   // seed 0 = acak dari waktu sistem

    // ------------------------------------------------------------------
    // 4. Loop semua bin dan aplikasikan smearing dengan cut energi
    // ------------------------------------------------------------------
    int nbins = h_raw->GetNbinsX();

    for (int i = 1; i <= nbins; ++i) {
        double E_center  = h_raw->GetBinCenter(i);
        double bin_width = h_raw->GetBinWidth(i);
        int    n_events  = (int)std::round(h_raw->GetBinContent(i));

        if (n_events <= 0 || E_center <= 0) continue;

        // CUT ENERGY RANGE: Hanya proses bin di dalam rentang [cut_min, cut_max]
        if (cut_max > cut_min) {
            if (E_center < cut_min || E_center > cut_max) continue;
        }

        for (int ev = 0; ev < n_events; ++ev) {
            // Distribusikan secara merata di dalam lebar bin
            double E_awal = E_center + rng.Uniform(-bin_width / 2.0, bin_width / 2.0);

            // Filter cut jika E_awal berada di luar rentang cut
            if (cut_max > cut_min) {
                if (E_awal < cut_min || E_awal > cut_max) continue;
            }

            // Sigma skala dengan sqrt(E) sesuai statistik scintilasi
            double sigma_E = sigma_ref * std::sqrt(E_awal / E_ref);

            // Terapkan smearing Gaussian
            double E_baru = rng.Gaus(E_awal, sigma_E);

            // Potong energi: hanya simpan event yang masuk dalam rentang cut
            if (cut_max > cut_min) {
                if (E_baru < cut_min || E_baru > cut_max) continue;
            }

            h_smeared->Fill(E_baru);
        }
    }

    // ------------------------------------------------------------------
    // 5. Fitting Gaussian pada rentang pemotongan energi [cut_min, cut_max]
    // ------------------------------------------------------------------
    TF1*   gausFit   = nullptr;
    double mean      = 0.0;
    double mean_err  = 0.0;
    double sigma     = 0.0;
    double sigma_err = 0.0;
    double chi2_ndf  = 0.0;
    double prob      = 0.0;

    if (cut_max > cut_min) {
        // Matikan kotak statistik fitting pada canvas (nilai hanya di terminal & legend)
        gStyle->SetOptFit(0);

        gausFit = new TF1("gausFit", "gaus", cut_min, cut_max);
        gausFit->SetLineColor(kBlue);
        gausFit->SetLineWidth(2);

        // Estimasi parameter awal agar fitting langsung konvergen optimal
        double amp_init   = h_smeared->GetMaximum();
        double mean_init  = (E_ref >= cut_min && E_ref <= cut_max) ? E_ref : (cut_min + cut_max) / 2.0;
        double sigma_init = (resolution * E_ref) / 2.3548;

        gausFit->SetParameters(amp_init, mean_init, sigma_init);

        // Eksekusi fit: 'R' = rentang TF1, 'Q' = quiet (dump Minuit dimatikan), 'S' = simpan result
        TFitResultPtr fitResult = h_smeared->Fit(gausFit, "R Q S");

        // Ambil nilai parameter, error, dan probabilitas
        double constant     = gausFit->GetParameter(0);
        double constant_err = gausFit->GetParError(0);
        mean                = gausFit->GetParameter(1);
        mean_err            = gausFit->GetParError(1);
        sigma               = gausFit->GetParameter(2);
        sigma_err           = gausFit->GetParError(2);
        double fwhm         = 2.35482 * sigma;
        double fwhm_err     = 2.35482 * sigma_err;
        double res_calc     = (mean > 0) ? (fwhm / mean) * 100.0 : 0.0;
        double chi2         = gausFit->GetChisquare();
        int    ndf          = gausFit->GetNDF();
        chi2_ndf            = (ndf > 0) ? (chi2 / ndf) : 0.0;
        prob                = gausFit->GetProb();

        // Cetak ringkasan nilai hasil fitting ke terminal saja
        cout << "\n=================== HASIL FITTING GAUSSIAN (SMEARED) ===================" << endl;
        cout << " File Target       : " << fileName << endl;
        cout << " Histogram         : " << histName << " (Smeared)" << endl;
        cout << " Range Cut/Fit     : [" << cut_min << ", " << cut_max << "] keV" << endl;
        cout << " Resolusi Input    : " << (resolution * 100.0) << " % (Ref: " << E_ref << " keV)" << endl;
        cout << "------------------------------------------------------------------------" << endl;
        cout << fixed << setprecision(3);
        cout << " Constant (Tinggi) : " << constant << " +/- " << constant_err << endl;
        cout << " Mean (Posisi)     : " << mean << " +/- " << mean_err << " keV" << endl;
        cout << " Sigma             : " << sigma << " +/- " << sigma_err << " keV" << endl;
        cout << " FWHM              : " << fwhm << " +/- " << fwhm_err << " keV" << endl;
        cout << " Resolusi Dihitung : " << res_calc << " %" << endl;
        cout << " Chi2 / NDF        : " << chi2 << " / " << ndf << " (" << chi2_ndf << ")" << endl;
        cout << " Prob (p-value)    : " << prob << endl;
        cout << " Status Fit        : " << (fitResult->IsValid() ? "CONVERGED / SUCCESS" : "FAILED") << endl;
        cout << "========================================================================\n" << endl;
    }

    // ------------------------------------------------------------------
    // 6. Gambar hasilnya
    // ------------------------------------------------------------------
    TCanvas* c1 = new TCanvas("c1", "Photopeak Detector 0", 900, 600);
    c1->SetGrid();

    h_smeared->SetStats(0);
    h_smeared->Draw("E1");
    if (gausFit) {
        gausFit->Draw("SAME");
    }

    // Rentang sumbu X sesuai rentang energi yang dipotong
    if (cut_max > cut_min) {
        h_smeared->GetXaxis()->SetRangeUser(cut_min, cut_max);
    }

    // Legenda visual dengan informasi Mean, Sigma, Chi2/NDF, dan Prob
    TLegend* leg = new TLegend(0.57, 0.60, 0.88, 0.88);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextSize(0.028);
    leg->AddEntry(h_smeared, "Smeared Data", "l");
    if (gausFit) {
        leg->AddEntry(gausFit, "Gaussian Fit", "l");
        leg->AddEntry((TObject*)0, Form("Mean = %.2f #pm %.2f keV", mean, mean_err), "");
        leg->AddEntry((TObject*)0, Form("#sigma = %.2f #pm %.2f keV", sigma, sigma_err), "");
        leg->AddEntry((TObject*)0, Form("#chi^{2}/ndf = %.2f", chi2_ndf), "");
        //leg->AddEntry((TObject*)0, Form("Prob = %.4f", prob), "");
    }
    leg->Draw();

    c1->Modified();
    c1->Update();

    // ------------------------------------------------------------------
    // 7. Simpan ke PNG
    // ------------------------------------------------------------------
    c1->Print(outFile);
    cout << "[INFO] Canvas disimpan ke: " << outFile << endl;
    //c1->Draw();
}

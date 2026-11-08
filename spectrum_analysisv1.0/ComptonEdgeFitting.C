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
#include "TFitResult.h"

using namespace std;

// Cs-137 (662 keV) memiliki Compton Edge teoritis sekitar 477.3 keV.
// Rentang cut_min dan cut_max disesuaikan di sekitar daerah tersebut.
void ComptonEdgeFitting(const char* fileName   = "spectrum_det0_5090_5092.root",
                        const char* histName   = "h_scan_5090",
                        double      resolution = 0.07,
                        double      E_ref      = 662.0,
                        double      cut_min    = 400.0,
                        double      cut_max    = 540.0,
                        const char* outFile    = "ComptonEdge_Smearing_Result_Detector_0.png")
{
    TFile* f = TFile::Open(fileName, "READ");
    if (!f || f->IsZombie()) {
        cout << "[ERROR] Gagal membuka file: " << fileName << endl;
        return;
    }

    TH1D* h_raw = (TH1D*)f->Get(histName);
    if (!h_raw) {
        cout << "[ERROR] Histogram '" << histName << "' tidak ditemukan!" << endl;
        f->Close();
        return;
    }
    h_raw->SetDirectory(0);
    f->Close();

    TH1D* h_smeared = (TH1D*)h_raw->Clone("h_smeared");
    h_smeared->Reset();
    h_smeared->Sumw2();
    h_smeared->SetTitle("Spectrum Compton Edge 662 keV - Detector 0");
    h_smeared->SetLineColor(kRed);
    h_smeared->SetLineWidth(2);

    double sigma_ref = (resolution * E_ref) / 2.3548;
    TRandom3 rng(0);

    // Loop smearing resolusi energi dengan batas sedikit lebih lebar dari rentang fit
    // agar bin di tepi cut_min/cut_max tidak kehilangan kontribusi event (edge effect)
    int nbins = h_raw->GetNbinsX();
    double margin = 3.0 * sigma_ref;

    for (int i = 1; i <= nbins; ++i) {
        double E_center  = h_raw->GetBinCenter(i);
        double bin_width = h_raw->GetBinWidth(i);
        int    n_events  = (int)std::round(h_raw->GetBinContent(i));

        if (n_events <= 0 || E_center <= 0) continue;
        if (cut_max > cut_min && (E_center < (cut_min - margin) || E_center > (cut_max + margin))) continue;

        for (int ev = 0; ev < n_events; ++ev) {
            double E_awal  = E_center + rng.Uniform(-bin_width / 2.0, bin_width / 2.0);
            if (E_awal <= 0) continue;

            double sigma_E = sigma_ref * std::sqrt(E_awal / E_ref);
            double E_baru  = rng.Gaus(E_awal, sigma_E);

            if (cut_max > cut_min && (E_baru < cut_min || E_baru > cut_max)) continue;
            h_smeared->Fill(E_baru);
        }
    }

    // Fitting Gaussian pada daerah Compton Edge
    TF1*   gausFit   = nullptr;
    double mean      = 0.0, mean_err  = 0.0;
    double sigma     = 0.0, sigma_err = 0.0;
    double chi2_ndf  = 0.0;

    if (cut_max > cut_min) {
        gStyle->SetOptFit(0);

        gausFit = new TF1("gausFit", "gaus", cut_min, cut_max);
        gausFit->SetLineColor(kBlue);
        gausFit->SetLineWidth(2);

        // Estimasi parameter awal di sekitar puncak lokal Compton Edge (~450-477 keV)
        int    bin_max    = h_smeared->GetMaximumBin();
        double amp_init   = h_smeared->GetBinContent(bin_max);
        double mean_init  = h_smeared->GetBinCenter(bin_max);
        double sigma_init = sigma_ref * std::sqrt(477.3 / E_ref);

        gausFit->SetParameters(amp_init, mean_init, sigma_init);

        TFitResultPtr fitResult = h_smeared->Fit(gausFit, "R Q S");

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
        double prob         = gausFit->GetProb();

        cout << "\n============= HASIL FITTING COMPTON EDGE (GAUSSIAN) =============" << endl;
        cout << " File Target       : " << fileName << endl;
        cout << " Histogram         : " << histName << " (Smeared)" << endl;
        cout << " Range Cut/Fit     : [" << cut_min << ", " << cut_max << "] keV" << endl;
        cout << " Resolusi Input    : " << (resolution * 100.0) << " % (Ref: " << E_ref << " keV)" << endl;
        cout << "-----------------------------------------------------------------" << endl;
        cout << fixed << setprecision(3);
        cout << " Constant (Tinggi) : " << constant << " +/- " << constant_err << endl;
        cout << " Mean (Posisi)     : " << mean << " +/- " << mean_err << " keV" << endl;
        cout << " Sigma             : " << sigma << " +/- " << sigma_err << " keV" << endl;
        cout << " FWHM              : " << fwhm << " +/- " << fwhm_err << " keV" << endl;
        cout << " Resolusi Dihitung : " << res_calc << " %" << endl;
        cout << " Chi2 / NDF        : " << chi2 << " / " << ndf << " (" << chi2_ndf << ")" << endl;
        cout << " Prob (p-value)    : " << prob << endl;
        cout << " Status Fit        : " << (fitResult->IsValid() ? "CONVERGED / SUCCESS" : "FAILED") << endl;
        cout << "=================================================================\n" << endl;
    }

    // Plotting dan penyimpanan hasil
    TCanvas* c1 = new TCanvas("c1", "Compton Edge Detector 0", 900, 600);
    c1->SetGrid();

    h_smeared->SetStats(0);
    h_smeared->Draw("E1");
    if (gausFit) gausFit->Draw("SAME");

    if (cut_max > cut_min) {
        h_smeared->GetXaxis()->SetRangeUser(cut_min, cut_max);
    }

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
    }
    leg->Draw();

    c1->Modified();
    c1->Update();
    c1->Print(outFile);
}
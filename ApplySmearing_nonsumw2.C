#include <iostream>
#include <cmath>
#include "TH1D.h"
#include "TCanvas.h"
#include "TRandom3.h"
#include "TDirectory.h"

using namespace std;

void ApplySmearing_nonsumw2() {
    // 1. Ambil histogram asli dari memori
    TH1D *h_raw = (TH1D*)gDirectory->Get("h_scan_5090");
    if (!h_raw) {
        cout << "Error: Histogram 'h_scan_1000' tidak ditemukan!" << endl;
        return;
    }

    // 2. Buat histogram baru untuk hasil smearing
    TH1D *h_smeared = (TH1D*)h_raw->Clone("h_smeared");
    h_smeared->Reset(); 
    //h_smeared->Sumw2(); 
    h_smeared->SetTitle("Detector 0 - Scan Point 1000 (Smeared);Energy (keV);Counts");
    h_smeared->SetLineColor(kRed);

    // 3. Parameter Resolusi (~7% pada 662 keV untuk NaI(Tl))
    double E_ref = 662.0; 
    double resolution = 0.07; 
    double fwhm_ref = resolution * E_ref;
    double sigma_ref = fwhm_ref / 2.3548;

    TRandom3 rand(0); 

    // 4. Loop ke semua bin dan aplikasikan smearing
    int nbins = h_raw->GetNbinsX();
    for (int i = 1; i <= nbins; ++i) {
        double E = h_raw->GetBinCenter(i);
        int counts = h_raw->GetBinContent(i);

        if (counts > 0 && E > 0) {
            double sigma_E = sigma_ref * sqrt(E / E_ref);
            for (int c = 0; c < counts; ++c) {
                double E_baru = rand.Gaus(E, sigma_E);
                h_smeared->Fill(E_baru);
            }
        }
    }

    // 5. Gambar hasilnya
    TCanvas *c1 = new TCanvas("c1", "Smearing Result", 800, 600);
    h_smeared->Draw();
}


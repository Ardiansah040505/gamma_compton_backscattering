#include <iostream>
#include <iomanip>
#include <cmath>
#include "TFile.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TRandom3.h"
#include "TSystem.h"
#include "TROOT.h"

using namespace std;

// =============================================================================
//  CompareDetectors_Backscatter.C
//
//  Membandingkan spektrum backscatter (~130-280 keV) dari semua detektor
//  (det0 - det5) pada setiap posisi scan (y = -10 mm hingga +10 mm).
//
//  CARA PAKAI:
//    root -l -b -q 'CompareDetectors_Backscatter.C'
//
//  OUTPUT:
//    result_all_fitting/backscatter_compare/
//    ├── Backscatter_Compare_pos5090_y-10mm.png
//    ├── Backscatter_Compare_pos5091_y-9mm.png
//    ├── ...
//    └── Backscatter_Compare_pos5110_y+10mm.png
// =============================================================================

// =============================================================================
// Fungsi Helper: Tentukan grup file berdasarkan scan point
// =============================================================================
TString GetFileGroup(int scanPoint)
{
    if      (scanPoint <= 5092) return "5090_5092";
    else if (scanPoint <= 5095) return "5093_5095";
    else if (scanPoint <= 5098) return "5096_5098";
    else if (scanPoint <= 5101) return "5099_5101";
    else if (scanPoint <= 5104) return "5102_5104";
    else if (scanPoint <= 5107) return "5105_5107";
    else                        return "5108_5110";
}

// =============================================================================
// Fungsi Helper: Smearing histogram dengan cut energi
// =============================================================================
TH1D* SmearAndCut(TH1D*       h_raw,
                  const char* newName,
                  double      cut_min,
                  double      cut_max,
                  double      resolution,
                  double      E_ref,
                  int         color)
{
    TH1D* h_smeared = (TH1D*)h_raw->Clone(newName);
    h_smeared->Reset();
    h_smeared->Sumw2();
    h_smeared->SetDirectory(0);
    h_smeared->SetLineColor(color);
    h_smeared->SetLineWidth(2);
    //h_smeared->SetOptStat(1);

    double sigma_ref = (resolution * E_ref) / 2.3548;
    TRandom3 rng(0);

    int nbins = h_raw->GetNbinsX();
    for (int i = 1; i <= nbins; ++i) {
        double E_center  = h_raw->GetBinCenter(i);
        double bin_width = h_raw->GetBinWidth(i);
        int    n_events  = (int)std::round(h_raw->GetBinContent(i));

        if (n_events <= 0 || E_center <= 0) continue;
        if (E_center < cut_min || E_center > cut_max) continue;

        for (int ev = 0; ev < n_events; ++ev) {
            double E_awal  = E_center + rng.Uniform(-bin_width / 2.0, bin_width / 2.0);
            if (E_awal <= 0 || E_awal < cut_min || E_awal > cut_max) continue;

            double sigma_E = sigma_ref * std::sqrt(E_awal / E_ref);
            double E_baru  = rng.Gaus(E_awal, sigma_E);

            if (E_baru < cut_min || E_baru > cut_max) continue;
            h_smeared->Fill(E_baru);
        }
    }

    return h_smeared;
}

// =============================================================================
// Fungsi Utama
// =============================================================================
void CompareDetectors_Backscatter(double resolution = 0.07,
                                  double E_ref      = 662.0,
                                  double cut_min    = 130.0,
                                  double cut_max    = 280.0)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(1111);
    gStyle->SetOptFit(1111);

    // Buat folder output
    const char* outDir = "result_all_fitting/backscatter_compare";
    gSystem->mkdir("result_all_fitting", kTRUE);
    gSystem->mkdir(outDir, kTRUE);

    // Definisi warna dan label per detektor
    const int  N_DET   = 6;
    int detColors[N_DET] = {kRed, kBlue, kGreen+2, kOrange+7, kViolet+1, kCyan+2};
    const char* detLabels[N_DET] = {
        "Detector 0", "Detector 1", "Detector 2",
        "Detector 3", "Detector 4", "Detector 5"
    };

    // Rentang posisi scan
    const int SCAN_START = 5090;
    const int SCAN_END   = 5110;
    const int SCAN_CENTER = 5100; // y = 0 mm

    int totalCanvas  = 0;
    int totalSkipped = 0;

    cout << "\n=============================================================" << endl;
    cout << "    PERBANDINGAN BACKSCATTER SEMUA DETEKTOR PER POSISI SCAN" << endl;
    cout << "=============================================================" << endl;
    cout << " Rentang Energi : [" << cut_min << ", " << cut_max << "] keV" << endl;
    cout << " Posisi Scan    : " << SCAN_START << " - " << SCAN_END
         << " (" << (SCAN_END - SCAN_START + 1) << " titik)" << endl;
    cout << " Output Folder  : " << outDir << "/" << endl;
    cout << "-------------------------------------------------------------" << endl;

    for (int scanPoint = SCAN_START; scanPoint <= SCAN_END; ++scanPoint) {
        int     ypos     = scanPoint - SCAN_CENTER;  // posisi y dalam mm
        TString group    = GetFileGroup(scanPoint);
        TString histName = Form("h_scan_%d", scanPoint);

        // Tampilkan progress
        cout << Form("[%2d/%2d] Scan %d (y = %+d mm) ... ",
                     scanPoint - SCAN_START + 1,
                     SCAN_END - SCAN_START + 1,
                     scanPoint, ypos) << flush;

        // Buat canvas baru untuk posisi ini
        TString canvasTitle = Form("Backscatter Comparison - Scan %d (y = %+d mm)",
                                   scanPoint, ypos);
        TCanvas* c = new TCanvas(Form("c_comp_%d", scanPoint), canvasTitle, 1000, 650);
        c->SetGrid();

        // Buat legenda
        TLegend* leg = new TLegend(0.68, 0.55, 0.90, 0.88);
        leg->SetBorderSize(0);
        leg->SetFillStyle(0);
        leg->SetTextSize(0.028);
        leg->SetHeader(Form("Posisi y = %+d mm", ypos), "C");

        bool firstDraw = true;
        int  nSuccess  = 0;
        double globalMax = 0.0;

        // --- Pass 1: Kumpulkan semua histogram yang berhasil
        vector<TH1D*> hList;
        for (int det = 0; det < N_DET; ++det) {
            TString fileName = Form("spectrum_det%d_%s.root", det, group.Data());

            TFile* fin = TFile::Open(fileName.Data(), "READ");
            if (!fin || fin->IsZombie()) {
                hList.push_back(nullptr);
                if (fin) fin->Close();
                continue;
            }

            TH1D* h_raw = (TH1D*)fin->Get(histName.Data());
            if (!h_raw) {
                fin->Close();
                hList.push_back(nullptr);
                continue;
            }
            h_raw->SetDirectory(0);
            fin->Close();
            delete fin;

            TString sName = Form("h_smeared_det%d_pos%d", det, scanPoint);
            TH1D* h_sm = SmearAndCut(h_raw, sName.Data(), cut_min, cut_max,
                                      resolution, E_ref, detColors[det]);
            delete h_raw;
            hList.push_back(h_sm);

            // Lacak nilai maksimum global untuk skala sumbu Y
            double mx = h_sm->GetMaximum();
            if (mx > globalMax) globalMax = mx;
            nSuccess++;
        }

        if (nSuccess == 0) {
            cout << "[SEMUA DETEKTOR GAGAL DIBUKA]" << endl;
            delete c;
            totalSkipped++;
            continue;
        }

        // --- Pass 2: Gambar
        for (int det = 0; det < N_DET; ++det) {
            TH1D* h_sm = hList[det];
            if (!h_sm) continue;

            // Set judul & sumbu pada histogram pertama
            if (firstDraw) {
                h_sm->SetTitle(Form("Backscatter Comparison - Scan Point %d (y = %+d mm);Energy (keV);Counts",
                                    scanPoint, ypos));
                h_sm->GetXaxis()->SetRangeUser(cut_min, cut_max);
                h_sm->GetYaxis()->SetRangeUser(0, globalMax * 1.20);
                h_sm->SetStats(0);
                h_sm->Draw("HIST");
                firstDraw = false;
            } else {
                h_sm->Draw("HIST SAME");
            }

            leg->AddEntry(h_sm, detLabels[det], "l");
        }

        leg->Draw();
        c->Modified();
        c->Update();

        // Simpan ke PNG
        TString outPng;
        if (ypos >= 0) {
            outPng = Form("%s/Backscatter_Compare_pos%d_y+%02dmm.png", outDir, scanPoint, ypos);
        } else {
            outPng = Form("%s/Backscatter_Compare_pos%d_y-%02dmm.png", outDir, scanPoint, -ypos);
        }
        c->Print(outPng.Data());

        // Bersihkan memori
        for (auto h : hList) { if (h) delete h; }
        delete c;
        totalCanvas++;

        cout << "OK" << endl;
    }

    cout << "-------------------------------------------------------------" << endl;
    cout << " SELESAI!" << endl;
    cout << " Canvas dihasilkan : " << totalCanvas << " gambar" << endl;
    if (totalSkipped > 0)
        cout << " Posisi dilewati   : " << totalSkipped << " (file tidak ditemukan)" << endl;
    cout << " Lokasi output     : " << outDir << "/" << endl;
    cout << "=============================================================\n" << endl;
}


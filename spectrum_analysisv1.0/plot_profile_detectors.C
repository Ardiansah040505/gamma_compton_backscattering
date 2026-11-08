#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include "TFile.h"
#include "TH1D.h"
#include "TProfile.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TList.h"
#include "TROOT.h"
#include "TKey.h"
#include "TAxis.h"

using namespace std;

// Ekstraksi nilai koordinat x dari title histogram atau kalkulasi dari scanPoint
double ExtractPositionX(const char* title, int scanPoint)
{
    if (title && strlen(title) > 0) {
        string tStr(title);
        size_t xPosIdx = tStr.find("x=");
        if (xPosIdx != string::npos) {
            size_t start = xPosIdx + 2;
            size_t end = tStr.find("mm", start);
            if (end != string::npos) {
                try {
                    string valStr = tStr.substr(start, end - start);
                    valStr.erase(remove_if(valStr.begin(), valStr.end(), ::isspace), valStr.end());
                    return stod(valStr);
                } catch (...) {
                    // Fallback jika parsing gagal
                }
            }
        }
    }
    // Fallback formula: grid 101x101 (-50 mm sampai +50 mm)
    return (double)(scanPoint / 101) - 50.0;
}

void plot_profile_detectors(const char* inputSource = "result_all_fitting/backscatter/backscatter_fitting_summary.csv")
{
    // 1. Konfigurasi mode batch dan style ROOT
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gStyle->SetGridColor(kGray);

    // Tentukan direktori output berdasarkan sumber data
    TString srcStr(inputSource);
    bool isBackscatter = srcStr.Contains("backscatter");
    TString baseOutDir = isBackscatter ? "result_all_fitting/backscatter/profile_detectors"
                                       : "result_all_fitting/profile_detectors";
    TString perDetDir = Form("%s/per_detector", baseOutDir.Data());
    gSystem->mkdir(baseOutDir.Data(), kTRUE);
    gSystem->mkdir(perDetDir.Data(), kTRUE);

    // 2. Konfigurasi detektor dan binning
    const int N_DET = 6;
    int nBins = 101;
    double xMin = -50.5;
    double xMax = 50.5;

    Color_t detColors[N_DET] = {kRed, kBlue, kGreen+2, kOrange+7, kViolet+1, kCyan+2};
    int detMarkers[N_DET]    = {20, 21, 22, 23, 33, 34};
    const char* detLabels[N_DET] = {
        "Detector 0", "Detector 1", "Detector 2",
        "Detector 3", "Detector 4", "Detector 5"
    };

    // Inisialisasi TProfile untuk ke-6 detektor
    vector<TProfile*> prof(N_DET, nullptr);
    for (int d = 0; d < N_DET; ++d) {
        prof[d] = new TProfile(Form("prof_det%d", d),
                               Form("Detector %d", d),
                               nBins, xMin, xMax, "");
        prof[d]->SetDirectory(0);
    }

    // 3. Pemuatan data: dari CSV jika ada, atau fallback ke file ROOT langsung
    bool useCSV = false;
    if (inputSource && strlen(inputSource) > 0) {
        if (srcStr.EndsWith(".csv") && !gSystem->AccessPathName(inputSource)) {
            useCSV = true;
        }
    }

    int totalPointsLoaded = 0;

    if (useCSV) {
        cout << "\n=============================================================" << endl;
        cout << "  MEMBACA DATA FIT DARI CSV: " << inputSource << endl;
        cout << "=============================================================" << endl;

        ifstream file(inputSource);
        if (!file.is_open()) {
            cerr << "[ERROR] Gagal membuka file CSV: " << inputSource << endl;
            return;
        }

        string line;
        if (!getline(file, line)) {
            cerr << "[ERROR] File CSV kosong!" << endl;
            file.close();
            return;
        }

        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string token;
            vector<string> cols;
            while (getline(ss, token, ',')) {
                cols.push_back(token);
            }

            if (cols.size() < 12) continue;

            int det       = stoi(cols[2]);
            int scanPoint = stoi(cols[3]);
            double mean   = stod(cols[4]);
            string status = cols[11];

            // Hapus spasi/newline dari status jika ada
            status.erase(remove_if(status.begin(), status.end(), ::isspace), status.end());

            if (status != "SUCCESS") continue;
            if (det < 0 || det >= N_DET) continue;

            double xPos = (double)(scanPoint / 101) - 50.0;
            prof[det]->Fill(xPos, mean);
            totalPointsLoaded++;
        }
        file.close();
        cout << "[INFO] Berhasil memuat " << totalPointsLoaded << " titik data valid dari CSV." << endl;
    } else {
        // Fallback: Membaca langsung seluruh file ROOT spectrum_det*.root
        TSystemDirectory dir(".", ".");
        TList* files = dir.GetListOfFiles();
        vector<TString> fileList;

        if (files) {
            TIter next(files);
            TSystemFile* file;
            while ((file = (TSystemFile*)next())) {
                TString fname = file->GetName();
                if (!file->IsDirectory() && fname.BeginsWith("spectrum_det") && fname.EndsWith(".root") && !fname.BeginsWith("smeared_")) {
                    fileList.push_back(fname);
                }
            }
        }

        sort(fileList.begin(), fileList.end());
        int totalFiles = fileList.size();
        if (totalFiles == 0) {
            cerr << "[ERROR] Tidak ditemukan file data!" << endl;
            return;
        }

        cout << "\n=============================================================" << endl;
        cout << "  MEMBACA SPEKTRUM DARI " << totalFiles << " FILE ROOT..." << endl;
        cout << "=============================================================" << endl;

        for (size_t fIdx = 0; fIdx < fileList.size(); ++fIdx) {
            TString curFileName = fileList[fIdx];
            int det = -1;
            size_t detPos = curFileName.Index("spectrum_det");
            if (detPos != kNPOS && curFileName.Length() > (detPos + 12)) {
                det = curFileName[detPos + 12] - '0';
            }

            if (det < 0 || det >= N_DET) continue;

            TFile* fin = TFile::Open(curFileName.Data(), "READ");
            if (!fin || fin->IsZombie()) continue;

            TIter nextKey(fin->GetListOfKeys());
            TKey* key;
            while ((key = (TKey*)nextKey())) {
                TObject* obj = key->ReadObj();
                if (obj && obj->InheritsFrom(TH1D::Class())) {
                    TH1D* h = (TH1D*)obj;
                    TString hName = h->GetName();

                    int scanPoint = 0;
                    size_t scanIdx = hName.Index("h_scan_");
                    if (scanIdx != kNPOS) {
                        TString numStr = hName(scanIdx + 7, hName.Length() - (scanIdx + 7));
                        scanPoint = numStr.Atoi();
                    }

                    double xPos = ExtractPositionX(h->GetTitle(), scanPoint);
                    double mean = h->GetMean();

                    prof[det]->Fill(xPos, mean);
                    totalPointsLoaded++;
                }
                delete obj;
            }
            fin->Close();
            delete fin;
        }
    }

    // 4. Periksa detektor mana saja yang memiliki data
    vector<int> activeDets;
    for (int d = 0; d < N_DET; ++d) {
        if (prof[d]->GetEntries() > 0) {
            activeDets.push_back(d);
            cout << Form("[INFO] %s: %lld titik terisi", detLabels[d], (long long)prof[d]->GetEntries()) << endl;
        }
    }

    if (activeDets.empty()) {
        cerr << "[ERROR] Tidak ada detektor yang memiliki data!" << endl;
        return;
    }

    // 5. Konversi TProfile ke TGraphErrors:
    //    Sumbu X = Mean Energy (keV)
    //    Sumbu Y = Posisi x (mm)
    vector<TGraphErrors*> grProf(N_DET, nullptr);
    for (int d : activeDets) {
        vector<double> gx, gy, gex, gey;
        for (int b = 1; b <= nBins; ++b) {
            if (prof[d]->GetBinEntries(b) > 0) {
                double meanEnergy = prof[d]->GetBinContent(b);
                double meanErr    = prof[d]->GetBinError(b);
                double posX       = prof[d]->GetBinCenter(b);

                gx.push_back(meanEnergy);
                gy.push_back(posX);
                gex.push_back(meanErr);
                gey.push_back(0.0);
            }
        }

        int nPts = gx.size();
        grProf[d] = new TGraphErrors(nPts, gx.data(), gy.data(), gex.data(), gey.data());
        grProf[d]->SetName(Form("gr_prof_det%d", d));
        grProf[d]->SetLineColor(detColors[d]);
        grProf[d]->SetMarkerColor(detColors[d]);
        grProf[d]->SetMarkerStyle(detMarkers[d]);
        grProf[d]->SetMarkerSize(0.9);
        grProf[d]->SetLineWidth(2);
    }

    // Judul sumbu grafik
    TString plotTitle = isBackscatter ? "Mean Backscatter Energy Profile vs Position x;Mean Backscatter Energy (keV);Position x (mm)"
                                      : "Mean Energy Profile vs Position x;Mean Energy (keV);Position x (mm)";

    // 6. Gambar dan Simpan Plot Gabungan
    TCanvas *cCombined = new TCanvas("c_prof_combined", "Mean Energy Profile vs Position x", 1000, 700);
    cCombined->SetGrid();
    cCombined->SetLeftMargin(0.12);
    cCombined->SetBottomMargin(0.12);

    TMultiGraph *mg = new TMultiGraph();
    mg->SetTitle(plotTitle.Data());

    TLegend *legend = new TLegend(0.72, 0.65, 0.90, 0.88);
    legend->SetBorderSize(1);
    legend->SetFillColor(kWhite);
    legend->SetTextSize(0.028);

    for (int d : activeDets) {
        mg->Add(grProf[d], "LP");
        legend->AddEntry(grProf[d], Form("%s (N=%lld)", detLabels[d], (long long)prof[d]->GetEntries()), "lep");
    }

    mg->Draw("A");
    legend->Draw();
    cCombined->Modified();
    cCombined->Update();

    TString combinedPng = isBackscatter ? Form("%s/Profile_Backscatter_Combined.png", baseOutDir.Data())
                                        : Form("%s/Profile_MeanVsPosition_Combined.png", baseOutDir.Data());
    cCombined->Print(combinedPng.Data());
    cout << "\n[INFO] Saved Combined Profile Plot: " << combinedPng << endl;

    // 7. Gambar dan Simpan Plot Individual Per Detektor
    for (int d : activeDets) {
        TCanvas *cInd = new TCanvas(Form("c_ind_prof_det%d", d),
                                    Form("Mean Energy Profile - Detector %d", d),
                                    900, 600);
        cInd->SetGrid();
        cInd->SetLeftMargin(0.12);
        cInd->SetBottomMargin(0.12);

        TString indTitle = isBackscatter ? Form("Mean Backscatter Energy Profile vs Position x - %s;Mean Backscatter Energy (keV);Position x (mm)", detLabels[d])
                                         : Form("Mean Energy Profile vs Position x - %s;Mean Energy (keV);Position x (mm)", detLabels[d]);
        grProf[d]->SetTitle(indTitle.Data());
        grProf[d]->Draw("ALP");

        TLegend *indLeg = new TLegend(0.68, 0.78, 0.88, 0.88);
        indLeg->SetBorderSize(1);
        indLeg->SetFillColor(kWhite);
        indLeg->SetTextSize(0.030);
        indLeg->AddEntry(grProf[d], Form("%s (N=%lld)", detLabels[d], (long long)prof[d]->GetEntries()), "lep");
        indLeg->Draw();

        cInd->Modified();
        cInd->Update();

        TString indPng = isBackscatter ? Form("%s/Profile_Backscatter_Det%d.png", perDetDir.Data(), d)
                                       : Form("%s/Profile_MeanVsPosition_Det%d.png", perDetDir.Data(), d);
        cInd->Print(indPng.Data());
        cout << "[INFO] Saved Det " << d << " Profile Plot: " << indPng << endl;

        delete cInd;
        delete indLeg;
    }

    // 8. Simpan objek TProfile & TGraphErrors ke file ROOT
    TString rootOut = isBackscatter ? Form("%s/profiles_backscatter_detectors.root", baseOutDir.Data())
                                    : Form("%s/profiles_detectors.root", baseOutDir.Data());
    TFile *fout = TFile::Open(rootOut.Data(), "RECREATE");
    if (fout && !fout->IsZombie()) {
        fout->cd();
        for (int d : activeDets) {
            prof[d]->Write();
            grProf[d]->Write();
        }
        fout->Close();
        delete fout;
        cout << "[INFO] Saved ROOT File: " << rootOut << endl;
    }

    delete cCombined;
    delete legend;

    cout << "\n=============================================================" << endl;
    cout << "  PROFIL BACKSCATTER SELESAI DIPROSES DAN TERSIMPAN!         " << endl;
    cout << "  Folder Output: " << baseOutDir << "/" << endl;
    cout << "=============================================================\n" << endl;
}

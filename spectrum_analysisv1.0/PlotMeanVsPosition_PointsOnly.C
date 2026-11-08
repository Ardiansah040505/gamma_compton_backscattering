#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "TCanvas.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TMultiGraph.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TList.h"
#include "TROOT.h"
#include "TAxis.h"
#include "TFile.h"
#include "TH1D.h"
#include "TKey.h"

using namespace std;

struct DataPoint {
    int    det;
    int    scanPoint;
    double xPos;       // mm
    double mean;       // keV
};

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
                    // Hapus spasi jika ada
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

void PlotCombinedPointsOnly(const vector<DataPoint>& points,
                            const char* outDir,
                            const int* detColors,
                            const int* detMarkers,
                            const char* const* detLabels)
{
    vector<vector<DataPoint>> detPoints(6);
    for (const auto& pt : points) {
        if (pt.det >= 0 && pt.det < 6) {
            detPoints[pt.det].push_back(pt);
        }
    }

    // Urutkan titik berdasarkan xPos
    for (int d = 0; d < 6; ++d) {
        sort(detPoints[d].begin(), detPoints[d].end(), [](const DataPoint& a, const DataPoint& b) {
            if (a.xPos == b.xPos) return a.mean < b.mean;
            return a.xPos < b.xPos;
        });
    }

    TCanvas* c = new TCanvas("c_pts_combined", "Mean vs Position x", 1000, 700);
    c->SetGrid();

    TMultiGraph* mg = new TMultiGraph();
    TString title = "Mean Energy vs Position x;Mean Energy (keV);Position x (mm)";
    mg->SetTitle(title.Data());

    TLegend* leg = new TLegend(0.72, 0.65, 0.90, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillStyle(1001);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.028);

    for (int d = 0; d < 6; ++d) {
        int n = detPoints[d].size();
        if (n == 0) continue;

        vector<double> x(n), y(n);
        for (int i = 0; i < n; ++i) {
            x[i] = detPoints[d][i].mean;
            y[i] = detPoints[d][i].xPos;
        }

        TGraph* gr = new TGraph(n, x.data(), y.data());
        gr->SetName(Form("gr_pts_det%d", d));
        gr->SetMarkerColor(detColors[d]);
        gr->SetMarkerStyle(detMarkers[d]);
        gr->SetMarkerSize(1.0);

        mg->Add(gr, "P");
        leg->AddEntry(gr, detLabels[d], "p");
    }

    mg->Draw("AP");
    leg->Draw();

    c->Modified();
    c->Update();

    TString outPng = Form("%s/MeanVsPosition_PointsOnly_Combined.png", outDir);
    c->Print(outPng.Data());
    cout << "[INFO] Saved Combined Plot: " << outPng << endl;

    delete c;
}

void PlotIndividualPointsOnly(const vector<DataPoint>& points,
                              const char* baseOutDir,
                              const int* detColors,
                              const int* detMarkers)
{
    TString regDir = Form("%s/per_detector", baseOutDir);
    gSystem->mkdir(regDir.Data(), kTRUE);

    vector<vector<DataPoint>> detPoints(6);
    for (const auto& pt : points) {
        if (pt.det >= 0 && pt.det < 6) {
            detPoints[pt.det].push_back(pt);
        }
    }

    for (int d = 0; d < 6; ++d) {
        int n = detPoints[d].size();
        if (n == 0) continue;

        sort(detPoints[d].begin(), detPoints[d].end(), [](const DataPoint& a, const DataPoint& b) {
            if (a.xPos == b.xPos) return a.mean < b.mean;
            return a.xPos < b.xPos;
        });

        vector<double> x(n), y(n);
        for (int i = 0; i < n; ++i) {
            x[i] = detPoints[d][i].mean;
            y[i] = detPoints[d][i].xPos;
        }

        TCanvas* c = new TCanvas(Form("c_ind_pts_det%d", d),
                                 Form("Mean vs Position x - Detector %d", d),
                                 900, 600);
        c->SetGrid();

        TGraph* gr = new TGraph(n, x.data(), y.data());
        gr->SetName(Form("gr_ind_pts_det%d", d));
        gr->SetTitle(Form("Mean Energy vs Position x - Detector %d;Mean Energy (keV);Position x (mm)", d));
        gr->SetMarkerColor(detColors[d]);
        gr->SetMarkerStyle(detMarkers[d]);
        gr->SetMarkerSize(1.1);

        gr->Draw("AP");

        TLegend* leg = new TLegend(0.70, 0.78, 0.88, 0.88);
        leg->SetBorderSize(1);
        leg->SetFillStyle(1001);
        leg->SetFillColor(kWhite);
        leg->SetTextSize(0.032);
        leg->AddEntry(gr, Form("Detector %d (%d pts)", d, n), "p");
        leg->Draw();

        c->Modified();
        c->Update();

        TString outPng = Form("%s/MeanVsPosition_PointsOnly_Det%d.png", regDir.Data(), d);
        c->Print(outPng.Data());
        cout << "[INFO] Saved Det " << d << " Plot: " << outPng << endl;

        delete c;
        delete gr;
        delete leg;
    }
}

void PlotMeanVsPosition_PointsOnly(const char* inputSource = "")
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);

    const char* baseOutDir = "result_all_fitting/mean_vs_position_pointsonly";
    gSystem->mkdir(baseOutDir, kTRUE);

    vector<DataPoint> allPoints;

    // Cek apakah input berupa file CSV yang valid
    bool useCSV = false;
    if (inputSource && strlen(inputSource) > 0) {
        TString src(inputSource);
        if (src.EndsWith(".csv") && !gSystem->AccessPathName(inputSource)) {
            useCSV = true;
        }
    }

    if (useCSV) {
        cout << "[INFO] Membaca data dari file CSV: " << inputSource << endl;
        ifstream file(inputSource);
        string line;
        if (!getline(file, line)) {
            cerr << "[ERROR] File CSV kosong!" << endl;
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

            if (cols.size() < 19) continue;
            string fname  = cols[0];
            string hname  = cols[1];
            double mean   = stod(cols[7]);
            string status = cols[18];

            if (status != "SUCCESS") continue;

            int det = -1;
            size_t detPos = fname.find("spectrum_det");
            if (detPos != string::npos) {
                det = fname[detPos + 12] - '0';
            }

            int scanPoint = 0;
            size_t scanIdx = hname.find("h_scan_");
            if (scanIdx != string::npos) {
                scanPoint = stoi(hname.substr(scanIdx + 7));
            }

            double xPos = (double)(scanPoint / 101) - 50.0;

            DataPoint pt;
            pt.det = det;
            pt.scanPoint = scanPoint;
            pt.xPos = xPos;
            pt.mean = mean;
            allPoints.push_back(pt);
        }
        file.close();
    } else {
        // Mode default: Baca langsung seluruh file ROOT spectrum_det*.root
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
            cerr << "[ERROR] Tidak ditemukan file 'spectrum_det*.root' di folder ini!" << endl;
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

            if (det < 0 || det > 5) continue;

            TFile* fin = TFile::Open(curFileName.Data(), "READ");
            if (!fin || fin->IsZombie()) {
                cerr << "[WARNING] Gagal membuka file: " << curFileName << endl;
                continue;
            }

            TIter nextKey(fin->GetListOfKeys());
            TKey* key;
            int fileHists = 0;

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

                    DataPoint pt;
                    pt.det = det;
                    pt.scanPoint = scanPoint;
                    pt.xPos = xPos;
                    pt.mean = mean;
                    allPoints.push_back(pt);

                    fileHists++;
                }
                delete obj;
            }

            fin->Close();
            delete fin;

            if ((fIdx + 1) % 20 == 0 || fIdx + 1 == fileList.size()) {
                cout << Form("[INFO] Diproses: %3zu/%d file ROOT | Total spektrum: %zu",
                             fIdx + 1, totalFiles, allPoints.size()) << endl;
            }
        }
    }

    if (allPoints.empty()) {
        cerr << "[ERROR] Tidak ada titik data yang berhasil dimuat!" << endl;
        return;
    }

    const int detColors[6] = {kRed, kBlue, kGreen+2, kOrange+7, kViolet+1, kCyan+2};
    const int detMarkers[6] = {20, 21, 22, 23, 33, 34};
    const char* detLabels[6] = {
        "Detector 0", "Detector 1", "Detector 2",
        "Detector 3", "Detector 4", "Detector 5"
    };

    cout << "\n=============================================================" << endl;
    cout << "  MEMBUAT PLOT MEAN ENERGY VS POSITION X                     " << endl;
    cout << "  Total data points : " << allPoints.size() << endl;
    cout << "=============================================================" << endl;

    // 1. Plot Gabungan (6 detektor)
    PlotCombinedPointsOnly(allPoints, baseOutDir, detColors, detMarkers, detLabels);

    // 2. Plot Per Detektor Individual
    PlotIndividualPointsOnly(allPoints, baseOutDir, detColors, detMarkers);

    cout << "\n[INFO] Selesai! Semua grafik tersimpan di: " << baseOutDir << "/" << endl;
    cout << "=============================================================\n" << endl;
}

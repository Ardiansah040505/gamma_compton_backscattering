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
#include "TROOT.h"
#include "TAxis.h"

using namespace std;

struct DataPoint {
    int    det;
    int    scanPoint;
    double yPos;       // mm
    double mean;       // keV
};

void PlotCombinedPointsOnly(const string& regionName,
                            const vector<DataPoint>& points,
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

    TCanvas* c = new TCanvas(Form("c_pts_%s", regionName.c_str()),
                             Form("Mean vs Position (Points Only) - %s", regionName.c_str()),
                             1000, 700);
    c->SetGrid();

    TMultiGraph* mg = new TMultiGraph();
    TString title = Form("Mean Energy vs Simulation Position y (%s) [Points Only];Mean Energy (keV);Position y (mm)",
                         regionName.c_str());
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
            y[i] = detPoints[d][i].yPos;
        }

        TGraph* gr = new TGraph(n, x.data(), y.data());
        gr->SetName(Form("gr_pts_det%d_%s", d, regionName.c_str()));
        gr->SetMarkerColor(detColors[d]);
        gr->SetMarkerStyle(detMarkers[d]);
        gr->SetMarkerSize(1.3);

        // Hanya tambahkan marker points ("P"), tanpa garis ("L")
        mg->Add(gr, "P");
        leg->AddEntry(gr, detLabels[d], "p");
    }

    // Gambar MultiGraph dengan opsi "AP" (Axis + Points)
    mg->Draw("AP");
    leg->Draw();

    c->Modified();
    c->Update();

    TString outPng = Form("%s/MeanVsPosition_PointsOnly_%s.png", outDir, regionName.c_str());
    c->Print(outPng.Data());
    cout << "[INFO] Saved Combined: " << outPng << endl;

    delete c;
}

void PlotIndividualPointsOnly(const string& regionName,
                              const map<int, vector<DataPoint>>& detMap,
                              const char* baseOutDir,
                              const int* detColors,
                              const int* detMarkers)
{
    TString regDir = Form("%s/per_detector/%s", baseOutDir, regionName.c_str());
    gSystem->mkdir(regDir.Data(), kTRUE);

    for (int d = 0; d < 6; ++d) {
        auto it = detMap.find(d);
        if (it == detMap.end() || it->second.empty()) continue;

        const auto& pts = it->second;
        int n = pts.size();
        vector<double> x(n), y(n);
        for (int i = 0; i < n; ++i) {
            x[i] = pts[i].mean;
            y[i] = pts[i].yPos;
        }

        TCanvas* c = new TCanvas(Form("c_ind_pts_%s_det%d", regionName.c_str(), d),
                                 Form("Mean vs Position - %s Det %d (Points Only)", regionName.c_str(), d),
                                 900, 600);
        c->SetGrid();

        TGraph* gr = new TGraph(n, x.data(), y.data());
        gr->SetName(Form("gr_ind_pts_%s_det%d", regionName.c_str(), d));
        gr->SetTitle(Form("%s - Detector %d [Points Only];Mean Energy (keV);Position y (mm)", regionName.c_str(), d));
        gr->SetMarkerColor(detColors[d]);
        gr->SetMarkerStyle(detMarkers[d]);
        gr->SetMarkerSize(1.4);

        // Gambar titik saja ("AP")
        gr->Draw("AP");

        TLegend* leg = new TLegend(0.70, 0.78, 0.88, 0.88);
        leg->SetBorderSize(1);
        leg->SetFillStyle(1001);
        leg->SetFillColor(kWhite);
        leg->SetTextSize(0.032);
        leg->AddEntry(gr, Form("Detector %d", d), "p");
        leg->Draw();

        c->Modified();
        c->Update();

        TString outPng = Form("%s/MeanVsPosition_PointsOnly_%s_Det%d.png", regDir.Data(), regionName.c_str(), d);
        c->Print(outPng.Data());
        delete c;
        delete gr;
        delete leg;
    }
}

void PlotMeanVsPosition_PointsOnly(const char* csvPath = "result_all_fitting/fitting_summary.csv")
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);

    const char* baseOutDir = "result_all_fitting/mean_vs_position_pointsonly";
    gSystem->mkdir(baseOutDir, kTRUE);

    ifstream file(csvPath);
    if (!file.is_open()) {
        cerr << "[ERROR] Cannot open CSV file: " << csvPath << endl;
        return;
    }

    string line;
    if (!getline(file, line)) {
        cerr << "[ERROR] Empty CSV file: " << csvPath << endl;
        file.close();
        return;
    }

    map<string, vector<DataPoint>> regionData;
    map<string, map<int, vector<DataPoint>>> perDetData;

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string token;
        vector<string> cols;

        while (getline(ss, token, ',')) {
            cols.push_back(token);
        }

        if (cols.size() < 19) continue;

        string fname   = cols[0];
        string hname   = cols[1];
        string region  = cols[2];
        double mean    = stod(cols[7]);
        string status  = cols[18];

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

        double yPos = scanPoint - 5100; // mm

        DataPoint pt;
        pt.det = det;
        pt.scanPoint = scanPoint;
        pt.yPos = yPos;
        pt.mean = mean;

        regionData[region].push_back(pt);
        perDetData[region][det].push_back(pt);
    }

    file.close();

    const int detColors[6] = {kRed, kBlue, kGreen+2, kOrange+7, kViolet+1, kCyan+2};
    const int detMarkers[6] = {20, 21, 22, 23, 33, 34};
    const char* detLabels[6] = {
        "Detector 0", "Detector 1", "Detector 2",
        "Detector 3", "Detector 4", "Detector 5"
    };

    cout << "\n=============================================================" << endl;
    cout << "  PLOT MEAN ENERGY VS POSITION Y (POINTS ONLY / SCATTER)     " << endl;
    cout << "=============================================================" << endl;

    // 1. Plot Gabungan (6 detektor dalam 1 kanvas per region)
    for (const auto& kv : regionData) {
        PlotCombinedPointsOnly(kv.first, kv.second, baseOutDir, detColors, detMarkers, detLabels);
    }

    // 2. Plot Per Detektor Individual
    for (const auto& kv : perDetData) {
        PlotIndividualPointsOnly(kv.first, kv.second, baseOutDir, detColors, detMarkers);
    }

    cout << "[INFO] Semua grafik Points Only selesai diproses!" << endl;
    cout << "=============================================================\n" << endl;
}


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

void PlotMeanVsPosition_PerDetector(const char* csvPath = "result_all_fitting/fitting_summary.csv")
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);

    const char* baseOutDir = "result_all_fitting/mean_vs_position_per_detector";
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

    // map[region][det] = vector<DataPoint>
    map<string, map<int, vector<DataPoint>>> allData;

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

        allData[region][det].push_back(pt);
    }

    file.close();

    const int detColors[6] = {kRed, kBlue, kGreen+2, kOrange+7, kViolet+1, kCyan+2};
    const int detMarkers[6] = {20, 21, 22, 23, 33, 34};

    cout << "\n=============================================================" << endl;
    cout << "     PLOT MEAN ENERGY VS POSITION Y PER DETEKTOR             " << endl;
    cout << "=============================================================" << endl;

    int totalSaved = 0;

    for (auto& regPair : allData) {
        string region = regPair.first;
        TString regDir = Form("%s/%s", baseOutDir, region.c_str());
        gSystem->mkdir(regDir.Data(), kTRUE);

        for (int d = 0; d < 6; ++d) {
            vector<DataPoint>& pts = regPair.second[d];
            if (pts.empty()) continue;

            sort(pts.begin(), pts.end(), [](const DataPoint& a, const DataPoint& b) {
                return a.yPos < b.yPos;
            });

            int n = pts.size();
            vector<double> x(n), y(n);
            for (int i = 0; i < n; ++i) {
                x[i] = pts[i].mean;
                y[i] = pts[i].yPos;
            }

            TCanvas* c = new TCanvas(Form("c_%s_det%d", region.c_str(), d),
                                     Form("Mean vs Position - %s Det %d", region.c_str(), d),
                                     900, 600);
            c->SetGrid();

            TGraph* gr = new TGraph(n, x.data(), y.data());
            gr->SetName(Form("gr_%s_det%d", region.c_str(), d));
            gr->SetTitle(Form("%s - Detector %d;Mean Energy (keV);Position y (mm)", region.c_str(), d));
            gr->SetLineColor(detColors[d]);
            gr->SetLineWidth(2);
            gr->SetMarkerColor(detColors[d]);
            gr->SetMarkerStyle(detMarkers[d]);
            gr->SetMarkerSize(1.2);

            gr->Draw("ALP");

            TLegend* leg = new TLegend(0.70, 0.78, 0.88, 0.88);
            leg->SetBorderSize(1);
            leg->SetFillStyle(1001);
            leg->SetFillColor(kWhite);
            leg->SetTextSize(0.032);
            leg->AddEntry(gr, Form("Detector %d", d), "lp");
            leg->Draw();

            c->Modified();
            c->Update();

            TString outPng = Form("%s/MeanVsPosition_%s_Det%d.png", regDir.Data(), region.c_str(), d);
            c->Print(outPng.Data());
            delete c;
            delete gr;
            delete leg;
            totalSaved++;
        }
    }

    cout << "[INFO] Total gambar tersimpan: " << totalSaved << " PNG" << endl;
    cout << "=============================================================\n" << endl;
}


// =============================================================================
// cnr_sensitivity_plot.C
// Makro ROOT C untuk visualisasi Sensitivitas Deteksi CNR vs Lebar Retakan
// Proyek Deteksi Keretakan Pipa GCB (Gamma Compton Backscattering)
// Dibaca dari: hasil_batch_cnr/batch_cnr_summary.csv
// Output    : hasil_batch_cnr/cnr_detection_sensitivity_root.png
// Jalankan  : root -l -b -q cnr_sensitivity_plot.C
// =============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include "TCanvas.h"
#include "TGraph.h"
#include "TLine.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"

using namespace std;

void cnr_sensitivity_plot(
    const char* csvPath = "hasil_batch_cnr/batch_cnr_summary.csv",
    const char* outDir  = "hasil_batch_cnr"
)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gStyle->SetPadGridX(kTRUE);
    gStyle->SetPadGridY(kTRUE);
    gStyle->SetGridColor(kGray);
    gStyle->SetGridStyle(3);
    gStyle->SetGridWidth(1);
    gStyle->SetPadLeftMargin(0.13);
    gStyle->SetPadBottomMargin(0.13);
    gStyle->SetPadRightMargin(0.06);
    gStyle->SetPadTopMargin(0.09);

    gSystem->mkdir(outDir, kTRUE);

    // === Baca CSV ===
    ifstream file(csvPath);
    if (!file.is_open()) {
        cerr << "[ERROR] Tidak dapat membuka CSV: " << csvPath << endl;
        return;
    }

    string line;
    if (!getline(file, line)) {
        cerr << "[ERROR] CSV kosong: " << csvPath << endl;
        file.close();
        return;
    }

    stringstream hss(line);
    string token;
    vector<string> headers;
    while (getline(hss, token, ',')) {
        // Hapus trailing carriage return
        while (!token.empty() && (token.back() == '\r' || token.back() == ' ')) token.pop_back();
        headers.push_back(token);
    }

    int iSize = -1, iCNR = -1;
    for (int i = 0; i < (int)headers.size(); ++i) {
        if (headers[i] == "Crack_Size_cm") iSize = i;
        if (headers[i] == "CNR")           iCNR  = i;
    }

    if (iSize < 0 || iCNR < 0) {
        cerr << "[ERROR] Kolom Crack_Size_cm atau CNR tidak ditemukan di header CSV." << endl;
        file.close();
        return;
    }

    vector<double> vCrackSize, vCNR;

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        vector<string> cols;
        while (getline(ss, token, ',')) cols.push_back(token);
        if ((int)cols.size() <= max(iSize, iCNR)) continue;

        double sz  = stod(cols[iSize]);
        double cnr = stod(cols[iCNR]);

        vCrackSize.push_back(sz);
        vCNR.push_back(cnr);
    }
    file.close();

    int n = (int)vCrackSize.size();
    if (n == 0) {
        cerr << "[ERROR] Tidak ada data di CSV: " << csvPath << endl;
        return;
    }

    cout << "[INFO] Data terbaca: " << n << " titik data CNR." << endl;

    // === Bangun TGraph ===
    TGraph* grCNR = new TGraph(n);
    for (int i = 0; i < n; ++i) {
        grCNR->SetPoint(i, vCrackSize[i], vCNR[i]);
    }
    grCNR->SetName("grCNR");
    grCNR->SetLineColor(kRed + 1);
    grCNR->SetLineWidth(2);
    grCNR->SetMarkerColor(kRed + 1);
    grCNR->SetMarkerStyle(kFullCircle);
    grCNR->SetMarkerSize(1.1);

    // Batas sumbu
    double xMin = 0.0;
    double xMax = 2.6;
    double maxCNR = *max_element(vCNR.begin(), vCNR.end());
    double yMax = max(maxCNR, 6.0) * 1.18;

    // === Canvas ===
    TCanvas* c = new TCanvas("c_cnr", "CBST Detection Sensitivity", 820, 580);
    c->SetGrid();

    grCNR->Draw("ALP");
    grCNR->GetXaxis()->SetTitle("Crack Width (cm)");
    grCNR->GetYaxis()->SetTitle("Contrast-to-Noise Ratio (CNR)");
    grCNR->GetXaxis()->SetTitleSize(0.048);
    grCNR->GetYaxis()->SetTitleSize(0.048);
    grCNR->GetXaxis()->SetLabelSize(0.040);
    grCNR->GetYaxis()->SetLabelSize(0.040);
    grCNR->GetXaxis()->SetTitleOffset(1.1);
    grCNR->GetYaxis()->SetTitleOffset(1.2);
    grCNR->GetXaxis()->SetLimits(xMin, xMax);
    grCNR->SetMinimum(0.0);
    grCNR->SetMaximum(yMax);

    // Garis ambang batas Rose Criterion (CNR = 5)
    TLine* lineRose = new TLine(xMin, 5.0, xMax, 5.0);
    lineRose->SetLineColor(kBlue + 1);
    lineRose->SetLineWidth(2);
    lineRose->SetLineStyle(7);  // dashed
    lineRose->Draw("SAME");

    // === TLegend ===
    TLegend* leg = new TLegend(0.16, 0.70, 0.58, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillStyle(1001);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.032);
    leg->AddEntry(grCNR, "CBST Projection CNR", "lp");
    leg->AddEntry(lineRose, "Rose Criterion Limit (CNR = 5)", "l");
    leg->Draw();

    // === Judul manual dengan TLatex ===
    TLatex title;
    title.SetNDC();
    title.SetTextSize(0.042);
    title.SetTextFont(62);
    title.SetTextAlign(22);
    title.DrawLatex(0.52, 0.95, "CBST Detection Sensitivity (CNR vs. Crack Width)");

    // === Anotasi nilai CNR di tiap titik data (staggered) ===
    TLatex annot;
    annot.SetTextSize(0.027);
    annot.SetTextFont(42);
    annot.SetTextColor(kBlack);
    annot.SetTextAlign(22);

    for (int i = 0; i < n; ++i) {
        double xval = vCrackSize[i];
        double yval = vCNR[i];
        double yOff = (i % 2 == 0) ? 0.025 * yMax : 0.050 * yMax;
        annot.DrawLatex(xval, yval + yOff, Form("%.1f", yval));
    }

    c->Modified();
    c->Update();

    TString outPng = Form("%s/cnr_detection_sensitivity.png", outDir);
    c->Print(outPng.Data());
    cout << "[INFO] Grafik CNR disimpan ke: " << outPng << endl;

    delete c;
}


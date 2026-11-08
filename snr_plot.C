// =============================================================================
// snr_plot.C
// Makro ROOT C untuk visualisasi Signal-to-Noise Ratio (SNR) vs Lebar Retakan
// Proyek Deteksi Keretakan Pipa GCB (Gamma Compton Backscattering)
// Dibaca dari: hasil_snr/snr_summary_all_cracks.csv
// Output    : hasil_snr/snr_vs_crack_width.png
// Jalankan  : root -l -b -q snr_plot.C
// =============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
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

void snr_plot(
    const char* csvPath = "hasil_snr/snr_summary_all_cracks.csv",
    const char* outDir  = "hasil_snr"
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
    gStyle->SetLegendBorderSize(1);
    gStyle->SetLegendFillColor(kWhite);
    gStyle->SetLegendTextSize(0.030);

    gSystem->mkdir(outDir, kTRUE);

    // === Baca CSV ===
    ifstream file(csvPath);
    if (!file.is_open()) {
        cerr << "[ERROR] Tidak dapat membuka CSV: " << csvPath << endl;
        return;
    }

    string line;
    // Baca baris header
    if (!getline(file, line)) {
        cerr << "[ERROR] CSV kosong: " << csvPath << endl;
        file.close();
        return;
    }

    // Parse header -> cari indeks kolom yang dibutuhkan
    // Kolom: File, Crack_Size_cm, Metric, Defect_Mean, Defect_Std, Defect_SNR,
    //        Normal_Mean, Normal_Std, Normal_SNR, SNR_Ratio_to_Normal
    stringstream hss(line);
    string token;
    vector<string> headers;
    while (getline(hss, token, ',')) headers.push_back(token);

    int iSize = -1, iMetric = -1, iDefSNR = -1, iNormSNR = -1;
    for (int i = 0; i < (int)headers.size(); ++i) {
        if (headers[i] == "Crack_Size_cm")  iSize   = i;
        if (headers[i] == "Metric")         iMetric = i;
        if (headers[i] == "Defect_SNR")     iDefSNR = i;
        if (headers[i] == "Normal_SNR")     iNormSNR= i;
    }

    if (iSize < 0 || iMetric < 0 || iDefSNR < 0 || iNormSNR < 0) {
        cerr << "[ERROR] Kolom tidak ditemukan di header CSV." << endl;
        file.close();
        return;
    }

    // Kumpulkan data metrik roiCounts
    vector<double> vCrackSize, vDefSNR;
    double normalSNR = -1.0;

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        vector<string> cols;
        while (getline(ss, token, ',')) cols.push_back(token);
        if ((int)cols.size() <= iNormSNR) continue;

        string metric = cols[iMetric];
        // Strip carriage return jika ada
        if (!metric.empty() && metric.back() == '\r') metric.pop_back();
        if (metric != "roiCounts") continue;

        double sz      = stod(cols[iSize]);
        double defSNR  = stod(cols[iDefSNR]);
        double normSNR = stod(cols[iNormSNR]);

        vCrackSize.push_back(sz);
        vDefSNR.push_back(defSNR);
        if (normalSNR < 0.0) normalSNR = normSNR;  // Ambil nilai normal 1 kali
    }
    file.close();

    int n = (int)vCrackSize.size();
    if (n == 0) {
        cerr << "[ERROR] Tidak ada data roiCounts di CSV." << endl;
        return;
    }

    cout << "[INFO] Data terbaca: " << n << " titik (roiCounts)." << endl;

    // === Bangun TGraph ===
    TGraph* grDefect = new TGraph(n);
    for (int i = 0; i < n; ++i) {
        grDefect->SetPoint(i, vCrackSize[i], vDefSNR[i]);
    }
    grDefect->SetName("grDefect");
    grDefect->SetLineColor(kBlue + 1);
    grDefect->SetLineWidth(2);
    grDefect->SetMarkerColor(kBlue + 1);
    grDefect->SetMarkerStyle(kFullCircle);  // marker bulat penuh
    grDefect->SetMarkerSize(1.1);

    // Batas sumbu
    double xMin = 0.0;
    double xMax = 2.6;
    double yMax = *max_element(vDefSNR.begin(), vDefSNR.end());
    yMax = max(yMax, normalSNR) * 1.18;

    // === Canvas ===
    TCanvas* c = new TCanvas("c_snr", "SNR vs Crack Width", 820, 580);
    c->SetGrid();

    // Frame kosong untuk atur sumbu sebelum gambar graph
    grDefect->Draw("ALP");
    grDefect->GetXaxis()->SetTitle("Crack Width (cm)");
    grDefect->GetYaxis()->SetTitle("Signal-to-Noise Ratio (SNR)");
    grDefect->GetXaxis()->SetTitleSize(0.048);
    grDefect->GetYaxis()->SetTitleSize(0.048);
    grDefect->GetXaxis()->SetLabelSize(0.040);
    grDefect->GetYaxis()->SetLabelSize(0.040);
    grDefect->GetXaxis()->SetTitleOffset(1.1);
    grDefect->GetYaxis()->SetTitleOffset(1.2);
    grDefect->GetXaxis()->SetLimits(xMin, xMax);
    grDefect->SetMinimum(0.0);
    grDefect->SetMaximum(yMax);

    // Garis horizontal SNR normal (baseline)
    TLine* lineNorm = new TLine(xMin, normalSNR, xMax, normalSNR);
    lineNorm->SetLineColor(kGreen + 2);
    lineNorm->SetLineWidth(2);
    lineNorm->SetLineStyle(7);  // dashed
    lineNorm->Draw("SAME");

    // === TLegend ===
    TLegend* leg = new TLegend(0.52, 0.65, 0.91, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillStyle(1001);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.032);
    leg->AddEntry(grDefect, "Defect ROI SNR (roiCounts)", "lp");
    leg->AddEntry(lineNorm, Form("Normal Baseline (SNR = %.2f)", normalSNR), "l");
    leg->Draw();

    // === Judul manual dengan TLatex ===
    TLatex title;
    title.SetNDC();
    title.SetTextSize(0.042);
    title.SetTextFont(62);
    title.SetTextAlign(22);
    title.DrawLatex(0.52, 0.95, "Signal-to-Noise Ratio (SNR) across Defect Widths");

    // === Anotasi nilai SNR di tiap titik data (staggered) ===
    TLatex annot;
    annot.SetTextSize(0.028);
    annot.SetTextFont(42);
    annot.SetTextColor(kBlack);
    annot.SetTextAlign(22);

    for (int i = 0; i < n; ++i) {
        double xval = vCrackSize[i];
        double yval = vDefSNR[i];
        double yOff = (i % 2 == 0) ? 0.025 * yMax : 0.048 * yMax;
        annot.DrawLatex(xval, yval + yOff, Form("%.1f", yval));
    }

    c->Modified();
    c->Update();

    TString outPng = Form("%s/snr_vs_crack_width.png", outDir);
    c->Print(outPng.Data());
    cout << "[INFO] Grafik SNR disimpan ke: " << outPng << endl;

    delete c;
}


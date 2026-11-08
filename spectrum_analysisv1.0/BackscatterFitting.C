#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <set>
#include <string>
#include "TFile.h"
#include "TH1D.h"
#include "TF1.h"
#include "TStyle.h"
#include "TRandom3.h"
#include "TFitResult.h"
#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TList.h"
#include "TROOT.h"
#include "TKey.h"

using namespace std;

// Struktur untuk menyimpan hasil fit 1 histogram
struct BackscatterFitResult {
    TString fileName;
    TString histName;
    int     det;
    int     scanPoint;
    double  constant;
    double  constant_err;
    double  mean;
    double  mean_err;
    double  sigma;
    double  sigma_err;
    double  fwhm;
    double  fwhm_err;
    double  res_calc;
    double  chi2_ndf;
    double  prob;
    bool    isValid;
};

// Fungsi helper untuk memproses dan mem-fit 1 histogram backscatter (tanpa PNG)
BackscatterFitResult FitSingleBackscatter(TH1D*       h_raw,
                                         const char* fileName,
                                         const char* histName,
                                         double      resolution,
                                         double      E_ref,
                                         double      cut_min,
                                         double      cut_max)
{
    BackscatterFitResult res;
    res.fileName     = gSystem->BaseName(fileName);
    res.histName     = histName;
    res.det          = -1;
    res.scanPoint    = 0;
    res.constant     = res.constant_err = 0.0;
    res.mean         = res.mean_err     = 0.0;
    res.sigma        = res.sigma_err    = 0.0;
    res.fwhm         = res.fwhm_err     = 0.0;
    res.res_calc     = 0.0;
    res.chi2_ndf     = 0.0;
    res.prob         = 0.0;
    res.isValid      = false;

    // Ekstrak ID detektor
    size_t detPos = res.fileName.Index("spectrum_det");
    if (detPos != kNPOS && res.fileName.Length() > (detPos + 12)) {
        res.det = res.fileName[detPos + 12] - '0';
    }

    // Ekstrak scan point
    TString hNameStr(histName);
    size_t scanIdx = hNameStr.Index("h_scan_");
    if (scanIdx != kNPOS) {
        TString numStr = hNameStr(scanIdx + 7, hNameStr.Length() - (scanIdx + 7));
        res.scanPoint = numStr.Atoi();
    }

    // 1. Buat histogram hasil smearing
    TString smearedName = Form("%s_smeared_backscatter", histName);
    TH1D* h_smeared = (TH1D*)h_raw->Clone(smearedName.Data());
    h_smeared->Reset();
    h_smeared->Sumw2();
    h_smeared->SetDirectory(0);

    double sigma_ref = (resolution * E_ref) / 2.3548;
    TRandom3 rng(0);

    int nbins = h_raw->GetNbinsX();
    for (int i = 1; i <= nbins; ++i) {
        double E_center  = h_raw->GetBinCenter(i);
        double bin_width = h_raw->GetBinWidth(i);
        int    n_events  = (int)std::round(h_raw->GetBinContent(i));

        if (n_events <= 0 || E_center <= 0) continue;
        if (cut_max > cut_min && (E_center < cut_min || E_center > cut_max)) continue;

        for (int ev = 0; ev < n_events; ++ev) {
            double E_awal = E_center + rng.Uniform(-bin_width / 2.0, bin_width / 2.0);
            if (E_awal <= 0) continue;
            if (cut_max > cut_min && (E_awal < cut_min || E_awal > cut_max)) continue;

            double sigma_E = sigma_ref * std::sqrt(E_awal / E_ref);
            double E_baru  = rng.Gaus(E_awal, sigma_E);

            if (cut_max > cut_min && (E_baru < cut_min || E_baru > cut_max)) continue;
            h_smeared->Fill(E_baru);
        }
    }

    // 2. Fitting Gaussian pada peak backscatter
    gStyle->SetOptFit(0);
    TF1* gausFit = new TF1(Form("gaus_%s", histName), "gaus", cut_min, cut_max);

    // Estimasi parameter awal spesifik peak backscatter
    int bin_max = h_smeared->GetMaximumBin();
    double amp_init   = (bin_max > 0) ? h_smeared->GetBinContent(bin_max) : h_smeared->GetMaximum();
    double mean_init  = (bin_max > 0) ? h_smeared->GetBinCenter(bin_max)  : (cut_min + cut_max) / 2.0;
    double sigma_init = sigma_ref * std::sqrt(mean_init / E_ref);

    gausFit->SetParameters(amp_init, mean_init, sigma_init);

    TFitResultPtr fitResult = h_smeared->Fit(gausFit, "R Q S 0");

    res.isValid      = (fitResult.Get() != nullptr && fitResult->IsValid());
    res.constant     = gausFit->GetParameter(0);
    res.constant_err = gausFit->GetParError(0);
    res.mean         = gausFit->GetParameter(1);
    res.mean_err     = gausFit->GetParError(1);
    res.sigma        = gausFit->GetParameter(2);
    res.sigma_err    = gausFit->GetParError(2);
    res.fwhm         = 2.35482 * res.sigma;
    res.fwhm_err     = 2.35482 * res.sigma_err;
    res.res_calc     = (res.mean > 0) ? (res.fwhm / res.mean) * 100.0 : 0.0;
    double chi2      = gausFit->GetChisquare();
    int ndf          = gausFit->GetNDF();
    res.chi2_ndf     = (ndf > 0) ? (chi2 / ndf) : 0.0;
    res.prob         = gausFit->GetProb();

    delete gausFit;
    delete h_smeared;

    return res;
}

/**
 * Macro ROOT untuk fitting Gaussian pada Backscatter Peak spektrum gamma.
 * HANYA menghasilkan file CSV ringkasan fitting (tanpa gambar PNG).
 *
 * CARA PAKAI:
 *   1. Fit SEMUA file ROOT (auto-resume jika CSV sudah ada):
 *      root -l -b -q BackscatterFitting.C
 *      atau
 *      root -l -b -q 'BackscatterFitting.C("all")'
 *
 *   2. Fit spesifik file & histogram:
 *      root -l -b -q 'BackscatterFitting.C("spectrum_det0_0_510.root", "h_scan_0")'
 *
 *   3. Fit semua histogram dalam 1 file:
 *      root -l -b -q 'BackscatterFitting.C("spectrum_det0_0_510.root", "all")'
 */
void BackscatterFitting(const char* fileName   = "all",
                        const char* histName   = "all",
                        double      resolution = 0.07,
                        double      E_ref      = 662.0,
                        double      cut_min    = 130.0,
                        double      cut_max    = 280.0)
{
    gROOT->SetBatch(kTRUE);

    const char* baseOutDir = "result_all_fitting/backscatter";
    gSystem->mkdir("result_all_fitting", kTRUE);
    gSystem->mkdir(baseOutDir, kTRUE);

    TString csvPath = Form("%s/backscatter_fitting_summary.csv", baseOutDir);

    // Kumpulkan entri yang sudah pernah diproses di CSV (agar mendukung auto-resume)
    set<string> doneEntries;
    bool csvExists = !gSystem->AccessPathName(csvPath.Data());
    if (csvExists) {
        ifstream infile(csvPath.Data());
        string line;
        while (getline(infile, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string ftoken, htoken;
            if (getline(ss, ftoken, ',') && getline(ss, htoken, ',')) {
                doneEntries.insert(ftoken + ":" + htoken);
            }
        }
        infile.close();
    }

    // MODE BATCH: Proses semua file spectrum_det*.root
    if (TString(fileName) == "all" || strlen(fileName) == 0) {
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
            cerr << "[ERROR] Tidak ditemukan file 'spectrum_det*.root'!" << endl;
            return;
        }

        cout << "\n=============================================================" << endl;
        cout << "  BATCH FITTING BACKSCATTER PADA " << totalFiles << " FILE ROOT (CSV ONLY)" << endl;
        cout << "  Rentang Fit: [" << cut_min << ", " << cut_max << "] keV" << endl;
        if (!doneEntries.empty()) {
            cout << "  Auto-Resume: " << doneEntries.size() - 1 << " titik sudah selesai sebelumnya." << endl;
        }
        cout << "=============================================================" << endl;

        ofstream csvFile(csvPath.Data(), ios::app);
        if (!csvExists || doneEntries.empty()) {
            csvFile << "File,Histogram,Detector,ScanPoint,Mean_keV,Mean_Err,Sigma_keV,Sigma_Err,FWHM_keV,Resolution_Pct,Chi2_NDF,Status\n";
        }

        int totalNew = 0;
        int totalSkipped = 0;

        for (size_t fIdx = 0; fIdx < fileList.size(); ++fIdx) {
            TString curFile = fileList[fIdx];
            TString baseName = gSystem->BaseName(curFile.Data());

            TFile* fin = TFile::Open(curFile.Data(), "READ");
            if (!fin || fin->IsZombie()) continue;

            TIter nextKey(fin->GetListOfKeys());
            TKey* key;
            while ((key = (TKey*)nextKey())) {
                TObject* obj = key->ReadObj();
                if (obj && obj->InheritsFrom(TH1D::Class())) {
                    TH1D* h_raw = (TH1D*)obj;
                    TString hName = h_raw->GetName();

                    string keyId = string(baseName.Data()) + ":" + string(hName.Data());
                    if (doneEntries.count(keyId) > 0) {
                        totalSkipped++;
                        delete obj;
                        continue;
                    }

                    BackscatterFitResult res = FitSingleBackscatter(
                        h_raw, curFile.Data(), hName.Data(),
                        resolution, E_ref, cut_min, cut_max);

                    csvFile << res.fileName << "," << res.histName << "," << res.det << "," << res.scanPoint << ","
                            << fixed << setprecision(3) << res.mean << "," << res.mean_err << ","
                            << res.sigma << "," << res.sigma_err << "," << res.fwhm << ","
                            << res.res_calc << "," << res.chi2_ndf << ","
                            << (res.isValid ? "SUCCESS" : "FAILED") << "\n";
                    csvFile.flush();

                    doneEntries.insert(keyId);
                    totalNew++;
                }
                delete obj;
            }
            fin->Close();
            delete fin;

            if ((fIdx + 1) % 10 == 0 || fIdx + 1 == fileList.size()) {
                cout << Form("[INFO] File %3zu/%d | Tambahan baru: %d | Dilewati: %d",
                             fIdx + 1, totalFiles, totalNew, totalSkipped) << endl;
            }
        }
        csvFile.close();
        cout << "\n[INFO] Selesai! Total titik tersimpan di CSV: " << doneEntries.size() - 1 << endl;
        cout << "[INFO] File CSV: " << csvPath << endl;
        cout << "=============================================================\n" << endl;
        return;
    }

    // MODE FILE TUNGGAL
    TFile* f = TFile::Open(fileName, "READ");
    if (!f || f->IsZombie()) {
        cerr << "[ERROR] Gagal membuka file: " << fileName << endl;
        return;
    }

    TString baseName = gSystem->BaseName(fileName);

    ofstream csvFile(csvPath.Data(), ios::app);
    if (!csvExists || doneEntries.empty()) {
        csvFile << "File,Histogram,Detector,ScanPoint,Mean_keV,Mean_Err,Sigma_keV,Sigma_Err,FWHM_keV,Resolution_Pct,Chi2_NDF,Status\n";
    }

    if (TString(histName) == "all") {
        cout << "\n[INFO] Memproses seluruh histogram dalam file: " << fileName << " (CSV ONLY)" << endl;
        TIter nextKey(f->GetListOfKeys());
        TKey* key;
        int count = 0;
        while ((key = (TKey*)nextKey())) {
            TObject* obj = key->ReadObj();
            if (obj && obj->InheritsFrom(TH1D::Class())) {
                TH1D* h = (TH1D*)obj;
                TString curHName = h->GetName();
                BackscatterFitResult res = FitSingleBackscatter(
                    h, fileName, curHName.Data(), resolution, E_ref, cut_min, cut_max);

                csvFile << res.fileName << "," << res.histName << "," << res.det << "," << res.scanPoint << ","
                        << fixed << setprecision(3) << res.mean << "," << res.mean_err << ","
                        << res.sigma << "," << res.sigma_err << "," << res.fwhm << ","
                        << res.res_calc << "," << res.chi2_ndf << ","
                        << (res.isValid ? "SUCCESS" : "FAILED") << "\n";
                count++;
            }
            delete obj;
        }
        f->Close();
        delete f;
        csvFile.close();
        cout << "[INFO] Selesai memproses " << count << " histogram ke CSV: " << csvPath << endl;
        return;
    }

    // Default 1 histogram spesifik
    TH1D* h_raw = (TH1D*)f->Get(histName);
    if (!h_raw) {
        cerr << "[ERROR] Histogram '" << histName << "' tidak ditemukan dalam " << fileName << "!" << endl;
        f->Close();
        delete f;
        csvFile.close();
        return;
    }
    h_raw->SetDirectory(0);
    f->Close();
    delete f;

    cout << "\n=============================================================" << endl;
    cout << "  FITTING GAUSSIAN BACKSCATTER PEAK (CSV ONLY)               " << endl;
    cout << "  File Target   : " << fileName << endl;
    cout << "  Histogram     : " << histName << endl;
    cout << "  Rentang Fit   : [" << cut_min << ", " << cut_max << "] keV" << endl;
    cout << "=============================================================" << endl;

    BackscatterFitResult res = FitSingleBackscatter(
        h_raw, fileName, histName, resolution, E_ref, cut_min, cut_max);

    csvFile << res.fileName << "," << res.histName << "," << res.det << "," << res.scanPoint << ","
            << fixed << setprecision(3) << res.mean << "," << res.mean_err << ","
            << res.sigma << "," << res.sigma_err << "," << res.fwhm << ","
            << res.res_calc << "," << res.chi2_ndf << ","
            << (res.isValid ? "SUCCESS" : "FAILED") << "\n";
    csvFile.close();

    cout << fixed << setprecision(3);
    cout << " Constant (Tinggi) : " << res.constant << " +/- " << res.constant_err << endl;
    cout << " Mean (Posisi)     : " << res.mean << " +/- " << res.mean_err << " keV" << endl;
    cout << " Sigma             : " << res.sigma << " +/- " << res.sigma_err << " keV" << endl;
    cout << " FWHM              : " << res.fwhm << " +/- " << res.fwhm_err << " keV" << endl;
    cout << " Resolusi Dihitung : " << res.res_calc << " %" << endl;
    cout << " Chi2 / NDF        : " << res.chi2_ndf << endl;
    cout << " Prob (p-value)    : " << res.prob << endl;
    cout << " Status Fit        : " << (res.isValid ? "CONVERGED / SUCCESS" : "FAILED") << endl;
    cout << "[INFO] Hasil tersimpan ke: " << csvPath << endl;
    cout << "=============================================================\n" << endl;

    delete h_raw;
}

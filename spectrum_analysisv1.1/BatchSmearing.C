#include <iostream>                                                                                                                                                                                                                  
    #include <cmath>                                                                                                                                                                                                                     
    #include <TFile.h>                                                                                                                                                                                                                   
    #include <TH1D.h>                                                                                                                                                                                                                    
    #include <TRandom3.h>                                                                                                                                                                                                                
    #include <TKey.h>                                                                                                                                                                                                                    
    #include <TSystem.h>                                                                                                                                                                                                                 
    #include <TSystemDirectory.h>                                                                                                                                                                                                        
    #include <TList.h>                                                                                                                                                                                                                   
                                                                                                                                                                                                                                         
    using namespace std;                                                                                                                                                                                                                 
                                                                                                                                                                                                                                         
    // Fungsi untuk melakukan smearing pada 1 histogram                                                                                                                                                                                  
    TH1D* SmearHistogram(TH1D* h_raw, TRandom3 &rand) {                                                                                                                                                                                  
        TString newName = Form("%s_smeared", h_raw->GetName());                                                                                                                                                                          
        TString newTitle = Form("%s (Smeared)", h_raw->GetTitle());                                                                                                                                                                      
                                                                                                                                                                                                                                         
        TH1D *h_smeared = (TH1D*)h_raw->Clone(newName);                                                                                                                                                                                  
        h_smeared->Reset();                                                                                                                                                                                                              
        h_smeared->SetTitle(newTitle);                                                                                                                                                                                                   
                                                                                                                                                                                                                                         
        // Parameter resolusi (~7% pada 662 keV NaI(Tl))                                                                                                                                                                                 
        double E_ref = 662.0;                                                                                                                                                                                                            
        double resolution = 0.07;                                                                                                                                                                                                        
        double fwhm_ref = resolution * E_ref;                                                                                                                                                                                            
        double sigma_ref = fwhm_ref / 2.3548;                                                                                                                                                                                            
                                                                                                                                                                                                                                         
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
        return h_smeared;                                                                                                                                                                                                                
    }                                                                                                                                                                                                                                    
                                                                                                                                                                                                                                         
    // 1. Fungsi untuk memproses 1 file                                                                                                                                                                                                  
    void ProcessOneFile(const char* inputFile, const char* outputFile) {                                                                                                                                                                 
        TFile *fin = TFile::Open(inputFile, "READ");                                                                                                                                                                                     
        if (!fin || fin->IsZombie()) {                                                                                                                                                                                                   
            cout << "[ERROR] Gagal membuka: " << inputFile << endl;                                                                                                                                                                      
            return;                                                                                                                                                                                                                      
        }                                                                                                                                                                                                                                
                                                                                                                                                                                                                                         
        TFile *fout = TFile::Open(outputFile, "RECREATE");                                                                                                                                                                               
        TRandom3 rand(0);                                                                                                                                                                                                                
                                                                                                                                                                                                                                         
        TIter next(fin->GetListOfKeys());                                                                                                                                                                                                
        TKey *key;                                                                                                                                                                                                                       
        int count = 0;                                                                                                                                                                                                                   
                                                                                                                                                                                                                                         
        cout << "Memproses file: " << inputFile << " -> " << outputFile << endl;                                                                                                                                                         
                                                                                                                                                                                                                                         
        while ((key = (TKey*)next())) {                                                                                                                                                                                                  
            TObject *obj = key->ReadObj();
            if (obj->InheritsFrom(TH1D::Class())) {
                TH1D *h_raw = (TH1D*)obj;
                TH1D *h_smeared = SmearHistogram(h_raw, rand);
                
                fout->cd();
                h_smeared->Write();
                delete h_smeared;
                count++;
            }
            delete obj;
        }
  
        fout->Close();
        fin->Close();
        delete fout;
        delete fin;
        cout << "Selesai! " << count << " histogram berhasil di-smear.\n" << endl;
    }
  
    // 2. Fungsi utama yang bisa menerima parameter atau memproses SEMUA file
    void BatchSmearing(const char* inputFile = "", const char* outputFile = "") {
        // Jika user memberikan nama file spesifik
        if (inputFile && strlen(inputFile) > 0) {
            TString out = outputFile;
            if (out.Length() == 0) {
                out = "smeared_" + TString(inputFile);
            }
            ProcessOneFile(inputFile, out.Data());
            return;
        }

        // Jika tanpa argumen, proses SEMUA file spectrum_det*.root di folder
        TSystemDirectory dir(".", ".");
        TList *files = dir.GetListOfFiles();

        if (!files) return;

        TIter next(files);
        TSystemFile *file;
        while ((file = (TSystemFile*)next())) {
            TString fname = file->GetName();
            // Hanya proses file yang diawali 'spectrum_det' dan berakhiran '.root'
            // serta bukan file hasil smearing
            if (!file->IsDirectory() && fname.BeginsWith("spectrum_det") && fname.EndsWith(".root") && !fname.BeginsWith("smeared_")) {
                TString outName = "smeared_" + fname;
                ProcessOneFile(fname.Data(), outName.Data());
            }
        }
    }

// C(++) headers
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>

// root headers
#include <TDirectory.h>
#include <TFile.h>
#include <TMath.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TTree.h>

using namespace std;

struct Hits {
    static constexpr int MAXHIT = 10000;
    int NHits;
    int PixX[MAXHIT];
    int PixY[MAXHIT];
    int Value[MAXHIT];
    int Timing[MAXHIT];
    int HitInCluster[MAXHIT];
    double PosX[MAXHIT];
    double PosY[MAXHIT];
    double PosZ[MAXHIT];
};

int Regions(const std::string &base, int bx0, int bx1, int Sy0, int Sy1,
            int Sa0, int Sa1)
{
    std::string path_input = base + ".root";
    std::string path_stime = base + "_Stime.root";
    std::string path_analog = base + "_Analog.root";

    Hits read;
    Hits write_stime;
    Hits write_analog;

    TFile *f = TFile::Open(path_input.c_str());
    TFile *fstime = TFile::Open(path_stime.c_str(), "RECREATE");
    TFile *fanalog = TFile::Open(path_analog.c_str(), "RECREATE");

    if (!f) {
        std::cerr << "Cannot open input file " << path_input << std::endl;
        return EXIT_FAILURE;
    }
    if (!fstime) {
        std::cerr << "Cannot open Stime output file " << path_stime
                  << std::endl;
        return EXIT_FAILURE;
    }
    if (!fanalog) {
        std::cerr << "Cannot open Analog output file " << path_analog
                  << std::endl;
        return EXIT_FAILURE;
    }

    TDirectory *d = (TDirectory *)f->Get("Plane0");
    TTree *t = (TTree *)d->Get("Hits");
    t->SetBranchAddress("NHits", &read.NHits);
    t->SetBranchAddress("PixX", read.PixX);
    t->SetBranchAddress("PixY", read.PixY);
    t->SetBranchAddress("Value", read.Value);
    t->SetBranchAddress("Timing", read.Timing);
    t->SetBranchAddress("HitInCluster", read.HitInCluster);
    t->SetBranchAddress("PosX", read.PosX);
    t->SetBranchAddress("PosY", read.PosY);
    t->SetBranchAddress("PosZ", read.PosZ);
    TTree *tEventIn = (TTree *)f->Get("Event");
    if (!tEventIn)
        cout << "WARNING: no EventTree found in input file..." << endl;

    TDirectory *dstime = fstime->mkdir("Plane0");
    dstime->cd();

    TTree *tstime = new TTree("Hits", "Hits");
    tstime->Branch("NHits", &write_stime.NHits, "NHits/I");
    tstime->Branch("PixX", write_stime.PixX, "HitPixX[NHits]/I");
    tstime->Branch("PixY", write_stime.PixY, "HitPixY[NHits]/I");
    tstime->Branch("Value", write_stime.Value, "HitValue[NHits]/I");
    tstime->Branch("Timing", write_stime.Timing, "HitTiming[NHits]/I");
    tstime->Branch("InCluster", write_stime.HitInCluster,
                   "InCluster[NHits]/I");
    tstime->Branch("PosX", write_stime.PosX, "HitPosX[NHits]/D");
    tstime->Branch("PosY", write_stime.PosY, "HitPosY[NHits]/D");
    tstime->Branch("PosZ", write_stime.PosZ, "HitPosZ[NHits]/D");
    TTree *tEventOut = tEventIn != 0 ? tEventIn->CloneTree() : NULL;

    TDirectory *danalog = fanalog->mkdir("Plane0");
    danalog->cd();

    TTree *tanalog = new TTree("Hits", "Hits");
    tanalog->Branch("NHits", &write_analog.NHits, "NHits/I");
    tanalog->Branch("PixX", write_analog.PixX, "HitPixX[NHits]/I");
    tanalog->Branch("PixY", write_analog.PixY, "HitPixY[NHits]/I");
    tanalog->Branch("Value", write_analog.Value, "HitValue[NHits]/I");
    tanalog->Branch("Timing", write_analog.Timing, "HitTiming[NHits]/I");
    tanalog->Branch("InCluster", write_analog.HitInCluster,
                    "InCluster[NHits]/I");
    tanalog->Branch("PosX", write_analog.PosX, "HitPosX[NHits]/D");
    tanalog->Branch("PosY", write_analog.PosY, "HitPosY[NHits]/D");
    tanalog->Branch("PosZ", write_analog.PosZ, "HitPosZ[NHits]/D");

    int nentries = t->GetEntries();
    cout << "Number of entries: " << nentries << endl;

    for (int i = 0; i < nentries; ++i) {
        int n_stime = 0;
        int n_analog = 0;
        t->GetEntry(i);
        write_stime.NHits = read.NHits;
        write_analog.NHits = read.NHits;
        for (int k = 0; k < read.NHits; ++k) {
            if (read.PixX[k] >= bx0 && read.PixX[k] <= bx1 &&
                read.PixY[k] >= Sy0 && read.PixY[k] <= Sy1) {
                n_stime += 1;
                write_stime.PixX[k] = read.PixX[k];
                write_stime.PixY[k] = read.PixY[k];
                write_stime.Value[k] = read.Value[k];
                write_stime.Timing[k] = read.Timing[k];
                write_stime.HitInCluster[k] = read.HitInCluster[k];
                write_stime.PosX[k] = read.PosX[k];
                write_stime.PosY[k] = read.PosY[k];
                write_stime.PosZ[k] = read.PosZ[k];
            }
            if (read.PixX[k] >= bx0 && read.PixX[k] <= bx1 &&
                read.PixY[k] >= Sa0 && read.PixY[k] <= Sa1) {
                n_analog += 1;
                write_analog.PixX[k] = read.PixX[k];
                write_analog.PixY[k] = read.PixY[k];
                write_analog.Value[k] = read.Value[k];
                write_analog.Timing[k] = read.Timing[k];
                write_analog.HitInCluster[k] = read.HitInCluster[k];
                write_analog.PosX[k] = read.PosX[k];
                write_analog.PosY[k] = read.PosY[k];
                write_analog.PosZ[k] = read.PosZ[k];
            }
        }
        // TODO this should be wrong since each k if either analog or stime
        write_stime.NHits = n_stime;
        write_analog.NHits = n_analog;

        fstime->cd();
        dstime->cd();
        tstime->Fill();

        fanalog->cd();
        danalog->cd();
        tanalog->Fill();
    }

    dstime->cd();
    tstime->Write();
    if (tEventOut != 0) {
        fstime->cd();
        tEventOut->Write();
    }
    danalog->cd();
    tanalog->Write();
    if (tEventOut != 0) {
        fstime->cd();
        tEventOut->Write();
    }

    cout << "Number of Stime entries: " << tstime->GetEntries() << endl;
    cout << "Number of Analog entries: " << tanalog->GetEntries() << endl;

    f->Close();
    fstime->Close();
    fanalog->Close();

    cout << "Analog and Stime pixels separated." << endl;
    return EXIT_SUCCESS;
}

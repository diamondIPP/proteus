// C(++) headers
#include <math.h>
#include <stdio.h>
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

#define MAXHIT 10000

using namespace std;

struct _hits {
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

// int merge(char * input, char * output) {
int Regions(std::string input, int bx0, int bx1, int Sy0, int Sy1, int Sa0,
            int Sa1)
{
    string output = input;
    string output1 = input;

    output += "_Stime.root";
    output1 += "_Analog.root";
    input += ".root";
    _hits read;
    _hits out_ana0;
    _hits out_ana;
    const char *in = input.c_str();

    TFile *f = TFile::Open(in);
    if (f == 0) {
        cout << "Cannot open, N.B. --No .root needed as input file--" << in
             << endl;
    }
    else {
        cout << "opening " << in << endl;
    }

    const char *ou = output.c_str();
    TFile *fnew = new TFile(ou, "RECREATE");
    if (fnew != 0) cout << "creating Stime file..." << endl;
    const char *out = output1.c_str();
    TFile *fnew1 = new TFile(out, "RECREATE");
    if (fnew1 != 0) cout << "creating Analog file..." << endl;

    TDirectory *d = (TDirectory *)f->Get("Plane0");
    TTree *t = (TTree *)d->Get("Hits");
    t->SetBranchAddress("NHits", &read.NHits);
    t->SetBranchAddress("PixX", read.PixX);
    t->SetBranchAddress("PixY", read.PixY);
    t->SetBranchAddress("Value", read.Value);
    t->SetBranchAddress("Timing", read.Timing);
    t->SetBranchAddress("InCluster", read.HitInCluster);
    t->SetBranchAddress("PosX", read.PosX);
    t->SetBranchAddress("PosY", read.PosY);
    t->SetBranchAddress("PosZ", read.PosZ);
    TTree *tEventIn = (TTree *)f->Get("Event");
    if (!tEventIn)
        cout << "WARNING: no EventTree found in input file..." << endl;

    TDirectory *dir = fnew->mkdir("Plane0");
    dir->cd();

    TTree *tnew = new TTree("Hits", "Hits");
    tnew->Branch("NHits", &out_ana0.NHits, "NHits/I");
    tnew->Branch("PixX", out_ana0.PixX, "HitPixX[NHits]/I");
    tnew->Branch("PixY", out_ana0.PixY, "HitPixY[NHits]/I");
    tnew->Branch("Value", out_ana0.Value, "HitValue[NHits]/I");
    tnew->Branch("Timing", out_ana0.Timing, "HitTiming[NHits]/I");
    tnew->Branch("InCluster", out_ana0.HitInCluster, "InCluster[NHits]/I");
    tnew->Branch("PosX", out_ana0.PosX, "HitPosX[NHits]/D");
    tnew->Branch("PosY", out_ana0.PosY, "HitPosY[NHits]/D");
    tnew->Branch("PosZ", out_ana0.PosZ, "HitPosZ[NHits]/D");
    TTree *tEventOut = tEventIn != 0 ? tEventIn->CloneTree() : NULL;

    TDirectory *dir1 = fnew1->mkdir("Plane0");
    dir1->cd();

    TTree *tnew1 = new TTree("Hits", "Hits");
    tnew1->Branch("NHits", &out_ana.NHits, "NHits/I");
    tnew1->Branch("PixX", out_ana.PixX, "HitPixX[NHits]/I");
    tnew1->Branch("PixY", out_ana.PixY, "HitPixY[NHits]/I");
    tnew1->Branch("Value", out_ana.Value, "HitValue[NHits]/I");
    tnew1->Branch("Timing", out_ana.Timing, "HitTiming[NHits]/I");
    tnew1->Branch("InCluster", out_ana.HitInCluster, "InCluster[NHits]/I");
    tnew1->Branch("PosX", out_ana.PosX, "HitPosX[NHits]/D");
    tnew1->Branch("PosY", out_ana.PosY, "HitPosY[NHits]/D");
    tnew1->Branch("PosZ", out_ana.PosZ, "HitPosZ[NHits]/D");

    int nentries = t->GetEntries();
    cout << "Number of entries: " << nentries << endl;
    int flag = 0, flag1 = 0;

    for (int i = 0; i < nentries; ++i) {
        flag = 0;
        flag1 = 0;
        t->GetEntry(i);
        out_ana0.NHits = read.NHits;
        out_ana.NHits = read.NHits;
        for (int k = 0; k < read.NHits; ++k) {
            if (read.PixX[k] >= bx0 && read.PixX[k] <= bx1 &&
                read.PixY[k] >= Sy0 && read.PixY[k] <= Sy1) {
                flag += 1;
                out_ana0.PixX[k] = read.PixX[k];
                out_ana0.PixY[k] = read.PixY[k];
                out_ana0.Value[k] = read.Value[k];
                out_ana0.Timing[k] = read.Timing[k];
                out_ana0.HitInCluster[k] = read.HitInCluster[k];
                out_ana0.PosX[k] = read.PosX[k];
                out_ana0.PosY[k] = read.PosY[k];
                out_ana0.PosZ[k] = read.PosZ[k];
            }
            if (read.PixX[k] >= bx0 && read.PixX[k] <= bx1 &&
                read.PixY[k] >= Sa0 && read.PixY[k] <= Sa1) {
                flag1 += 1;
                out_ana.PixX[k] = read.PixX[k];
                out_ana.PixY[k] = read.PixY[k];
                out_ana.Value[k] = read.Value[k];
                out_ana.Timing[k] = read.Timing[k];
                out_ana.HitInCluster[k] = read.HitInCluster[k];
                out_ana.PosX[k] = read.PosX[k];
                out_ana.PosY[k] = read.PosY[k];
                out_ana.PosZ[k] = read.PosZ[k];
            }
        }
        out_ana0.NHits = flag;
        out_ana.NHits = flag1;

        fnew->cd();
        dir->cd();
        tnew->Fill();

        fnew1->cd();
        dir1->cd();
        tnew1->Fill();
    }

    dir->cd();
    tnew->Write();
    if (tEventOut != 0) {
        fnew->cd();
        tEventOut->Write();
    }
    dir1->cd();
    tnew1->Write();
    if (tEventOut != 0) {
        fnew->cd();
        tEventOut->Write();
    }

    cout << "Number of entries output: " << tnew->GetEntries() << endl;
    cout << "Number of entries output1: " << tnew1->GetEntries() << endl;

    f->Close();
    fnew->Close();
    fnew1->Close();
    // new->Print();
    cout << "Analog and Stime pixels separated." << endl;
    return EXIT_SUCCESS;
}

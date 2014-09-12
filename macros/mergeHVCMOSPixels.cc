//C(++) headers
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <limits>


//root headers
#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TSystem.h> 
#include <TMath.h>
#include <TDirectory.h>

#define MAXHIT 1000

//first y value

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

int mergeHVCMOSPixels(char * input, char * output) {
	gROOT->Reset();
		
	_hits unmerged;
	_hits merged;
	

	TFile * f = TFile::Open(input);
	if (f == 0) cout<<"Cannot open "<<input<<endl;
	TDirectory * d = (TDirectory *)f->Get("Plane0");
	TTree * t = (TTree *)d->Get("Hits");
	
	TFile * fnew = new TFile(output, "RECREATE");
	//if (fnew != 0) cout<<"Merging..."<<endl;
	TDirectory * dnew = fnew->mkdir("Plane0");
	dnew->cd();
	TTree * m_pltree = new TTree("Hits", "Hits");
	
	t->SetBranchAddress("NHits", &unmerged.NHits);
	t->SetBranchAddress("PixX", unmerged.PixX);
	t->SetBranchAddress("PixY", unmerged.PixY);
	t->SetBranchAddress("Value", unmerged.Value);
	t->SetBranchAddress("Timing", unmerged.Timing);
	t->SetBranchAddress("InCluster", unmerged.HitInCluster);
	t->SetBranchAddress("PosX", unmerged.PosX);
	t->SetBranchAddress("PosY", unmerged.PosY);
	t->SetBranchAddress("PosZ", unmerged.PosZ);
	
	m_pltree->Branch("NHits", &unmerged.NHits, "NHits/I");
	m_pltree->Branch("PixX", unmerged.PixX, "HitPixX[NHits]/I");
	m_pltree->Branch("PixY", merged.PixY, "HitPixY[NHits]/I");
	m_pltree->Branch("Value", unmerged.Value, "HitValue[NHits]/I");
	m_pltree->Branch("Timing", unmerged.Timing, "HitTiming[NHits]/I");
	m_pltree->Branch("InCluster", unmerged.HitInCluster, "InCluster[NHits]/I");
	m_pltree->Branch("PosX", unmerged.PosX, "HitPosX[NHits]/D");
	m_pltree->Branch("PosY", unmerged.PosY, "HitPosY[NHits]/D");
	m_pltree->Branch("PosZ", unmerged.PosZ, "HitPosZ[NHits]/D");
	

	int nentries = t->GetEntries();
	int minY = std::numeric_limits<int>::max();
	//cout<<"Initialising minY "<<minY<<endl;
	
	for (int i = 0; i < nentries; ++i) {
		t->GetEntry(i);
		for (int j = 0; j < unmerged.NHits; ++j) {
			if (unmerged.PixY[j] < minY) minY = unmerged.PixY[j];
		}
	}
	//cout<<"Minimum PixY "<<minY<<endl;
	
	for (int i = 0; i < nentries; ++i) { //loop over entries
		t->GetEntry(i);
		for (int j = 0; j < unmerged.NHits; ++j) { //loop over NHits
			if ( unmerged.NHits > 0 ) {
				if (minY%2 == 0) merged.PixY[j] = TMath::FloorNint( unmerged.PixY[j]/2.0 );
				else merged.PixY[j] = TMath::FloorNint( (unmerged.PixY[j]-1.0)/2.0 );
				
				//cout << "Entry " << i << " | " << unmerged.PixY[j] << " --> " << merged.PixY[j] << endl;
			}
			
		}
		
		m_pltree->Fill();
	}
	
	fnew->Write();
	fnew->Print();
	//cout<<"Merging done."<<endl;
	return 42;
}

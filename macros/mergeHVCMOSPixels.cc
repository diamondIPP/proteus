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
#include <TString.h>
#include <TDirectory.h>
#include <TKey.h>

#define MAXHIT 1000

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

int mergeHVCMOSPixels(char *input, char *output) {
	gROOT->Reset();

	//
	// open input file and get TTrees
	//
	TFile *f = TFile::Open(input);
	if(f->IsZombie()){
		cout << "Cannot open '" << input << "' for reading. Exit." << endl;
		return 1;
	}

	//
	// open output file
	//
	TFile *fnew = new TFile(output, "RECREATE");
	if(fnew->IsZombie()){
		cout << "Cannot open '" << output << "' for writting. Exit." << endl;
		return 2;
	}  

	// look for trees in TFile that are not planes
	int ntrees = 0;
	TTree *tsumIn = (TTree*)f->Get("SummaryTree");
	TTree *tEventIn = (TTree*)f->Get("Event");  
	if(!tsumIn)	cout << "WARNING: no SummaryTree found in input file..." << endl; 
	else ntrees += 1;
	if(!tEventIn) cout << "WARNING: no EventTree found in input file..." << endl;
	else ntrees += 1;
  
	//
	//loop over all DUT planes
	//
	for (int plane = 0; plane < (f->GetNkeys()-ntrees); plane++) {
		TKey *key =  (TKey*)f->GetKey(Form("Plane%i",plane), 1);
		if (!key) {
			cout << "Unknown key." << endl;
			continue;			
		}
		
		TString name = key->GetName();

		TTree *t = (TTree*)f->Get(Form("%s/Hits",name.Data()));

		TDirectory *dnew = fnew->mkdir(name.Data());
		dnew->cd();
		
		//new Hits tree and clone Intercepts tree
		TTree *m_pltree = new TTree("Hits", "Hits");
		TTree *intercepts = ( (TTree*)f->Get(Form("%s/Intercepts",name.Data())) )->CloneTree();

		_hits unmerged;
		_hits merged;

		//
		// set branches
		//
		t->SetBranchAddress("NHits", &unmerged.NHits);
		t->SetBranchAddress("PixX", unmerged.PixX);
		t->SetBranchAddress("PixY", unmerged.PixY);
		t->SetBranchAddress("Value", unmerged.Value);
		t->SetBranchAddress("Timing", unmerged.Timing);
		t->SetBranchAddress("HitInCluster", unmerged.HitInCluster);
		t->SetBranchAddress("PosX", unmerged.PosX);
		t->SetBranchAddress("PosY", unmerged.PosY);
		t->SetBranchAddress("PosZ", unmerged.PosZ);

		m_pltree->Branch("NHits", &unmerged.NHits, "NHits/I"); // same as input
		m_pltree->Branch("PixX", unmerged.PixX, "HitPixX[NHits]/I"); // same as input
		m_pltree->Branch("PixY", merged.PixY, "HitPixY[NHits]/I"); 
		m_pltree->Branch("Value", unmerged.Value, "HitValue[NHits]/I"); // same as input
		m_pltree->Branch("Timing", unmerged.Timing, "HitTiming[NHits]/I"); // same as input
		m_pltree->Branch("HitInCluster", unmerged.HitInCluster, "HitInCluster[NHits]/I"); // same as input
		m_pltree->Branch("PosX", unmerged.PosX, "HitPosX[NHits]/D"); // same as input
		m_pltree->Branch("PosY", unmerged.PosY, "HitPosY[NHits]/D"); // same as input
		m_pltree->Branch("PosZ", unmerged.PosZ, "HitPosZ[NHits]/D"); // same as input

		//  int minY = std::numeric_limits<int>::max();
		int minY = 1e6; // allow to run macro non-compiled
		//cout<<"Initialising minY "<<minY<<endl;

		int nentries = t->GetEntriesFast();
		for (int i=0; i<nentries; ++i) {
			t->GetEntry(i);
			for (int j=0; j<unmerged.NHits; ++j) {
				if(unmerged.PixY[j] < minY) minY = unmerged.PixY[j];
			}
		} //cout<<"Minimum PixY "<<minY<<endl;

		for (int i=0; i<nentries; ++i) { //loop over entries
			t->GetEntry(i);
			for (int j=0; j<unmerged.NHits; ++j) { //loop over NHits
				if ( unmerged.NHits > 0 ) {
					if (minY%2 == 0) merged.PixY[j] = TMath::FloorNint( unmerged.PixY[j]/2.0 );
					else merged.PixY[j] = TMath::FloorNint( (unmerged.PixY[j]-1.0)/2.0 );
				//cout << "Entry " << i << " | " << unmerged.PixY[j] << " --> " << merged.PixY[j] << endl;
				}      
			}    
			m_pltree->Fill();
		}

		//
		// write trees and close output file
		//
		fnew->cd();
		dnew->cd();
		m_pltree->Write();
		intercepts->Write();
	}
	// end of loop over all DUT planes

	TTree *tsumOut = tsumIn!=0 ? tsumIn->CloneTree() : NULL;
	TTree *tEventOut = tEventIn!=0 ? tEventIn->CloneTree() : NULL;
	if(tsumOut!=0){ fnew->cd(); tsumOut->Write(); }
	if(tEventOut!=0){ fnew->cd(); tEventOut->Write(); }
	fnew->Print();
	fnew->Close();
	cout << "Merging done." << endl;

	//
	// close input file
	// 
	f->Close();  

	return 42; // ??? why
}

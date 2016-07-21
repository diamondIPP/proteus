//THIS MACRO IS USED ONLY FOR CCPD Version 4+

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
#include <TH2.h>
#include <TTree.h>
#include <TSystem.h> 
#include <TMath.h>
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

int mapping(char *input, char *output, const char *mapput = "mapping.root") {
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
	//
	// open mapping file
	//
	TFile *fmap = new TFile(mapput,"RECREATE");
	if(fmap->IsZombie()){
		cout << "Cannot open '" << mapput << "' for writting. Exit." << endl;
		return 3;
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

		//new Plane directories
		TDirectory *dnew = fnew->mkdir(name.Data());
		TDirectory *dmap = fmap->mkdir(name.Data());
		
		//get old Plane/Hits tree and clone Plane/Intercepts tree
		TTree *t = (TTree*)f->Get(Form("%s/Hits",name.Data()));		
		TTree *intercepts = ( (TTree*)f->Get(Form("%s/Intercepts",name.Data())) )->CloneTree();

		//new Hits tree
		dnew->cd();
		TTree *m_pltree = new TTree("Hits", "Hits");
	
		_hits unmapped;
		_hits mapped;

		//
		// set branches
		//
		t->SetBranchAddress("NHits", &unmapped.NHits);
		t->SetBranchAddress("PixX", unmapped.PixX);
		t->SetBranchAddress("PixY", unmapped.PixY);
		t->SetBranchAddress("Value", unmapped.Value);
		t->SetBranchAddress("Timing", unmapped.Timing);
		t->SetBranchAddress("HitInCluster", unmapped.HitInCluster);
		t->SetBranchAddress("PosX", unmapped.PosX);
		t->SetBranchAddress("PosY", unmapped.PosY);
		t->SetBranchAddress("PosZ", unmapped.PosZ);
		
		m_pltree->Branch("NHits", &unmapped.NHits, "NHits/I"); // same as input
		m_pltree->Branch("PixX", mapped.PixX, "HitPixX[NHits]/I");
		m_pltree->Branch("PixY", mapped.PixY, "HitPixY[NHits]/I"); 
		m_pltree->Branch("Value", unmapped.Value, "HitValue[NHits]/I"); // same as input
		m_pltree->Branch("Timing", unmapped.Timing, "HitTiming[NHits]/I"); // same as input
		m_pltree->Branch("HitInCluster", unmapped.HitInCluster, "HitInCluster[NHits]/I"); // same as input
		m_pltree->Branch("PosX", unmapped.PosX, "HitPosX[NHits]/D"); // same as input
		m_pltree->Branch("PosY", unmapped.PosY, "HitPosY[NHits]/D"); // same as input
		m_pltree->Branch("PosZ", unmapped.PosZ, "HitPosZ[NHits]/D"); // same as input
			
		//	int minY = std::numeric_limits<int>::max();
		int minY = 1000000; // allow to run macro non-compiled
		//cout<<"Initialising minY "<<minY<<endl;

		int nentries = t->GetEntriesFast();
		for (int i=0; i<nentries; ++i) {
			t->GetEntry(i);
			for (int j=0; j<unmapped.NHits; ++j) {
				if(unmapped.PixY[j]==0)continue;
				if(unmapped.PixY[j] < minY) minY = unmapped.PixY[j];
			}
		} //cout<<"Minimum PixY "<<minY<<endl;
		
		// someone please add axis title
		dmap->cd();
		TH2D *mapping_old=new TH2D("mapping_old","mapping_old",6,-0.5,6.5,16,-0.5,15.5);
		TH2D *mapping_new=new TH2D("mapping_new","mapping_new",12,-0.5,11.5,8,-0.5,7.5);
		TH2D *mapping_corX=new TH2D("mapping_corX","mapping_corX",6,-0.5,5.5,12,-0.5,11.5); 
		TH2D *mapping_corY=new TH2D("mapping_corY","mapping_corY",16,-0.5,15.5,8,-0.5,7.5); 

		for (int i=0; i<nentries; ++i) { //loop over entries
			t->GetEntry(i);
			for (int j=0; j<unmapped.NHits; ++j) { //loop over NHits
				if ( unmapped.NHits > 0 ) {
					if (minY%2 == 0) mapped.PixY[j] = TMath::FloorNint((unmapped.PixY[j]/*+1*/)/2.0 );
					else mapped.PixY[j] = TMath::FloorNint( (unmapped.PixY[j]-1.0)/2.0 );
				 // if((int)((unmapped.PixY[j]-minY)/2)%2!=0){
					if((int)((unmapped.PixX[j]))%2!=0){
						if((unmapped.PixY[j])%2 != 0) mapped.PixX[j] =2* unmapped.PixX[j]+1;
						if((unmapped.PixY[j])%2 == 0) mapped.PixX[j] =2* unmapped.PixX[j];
					 }

					if((int)((unmapped.PixX[j]))%2==0){
						if((unmapped.PixY[j])%2 != 0) mapped.PixX[j] =2* unmapped.PixX[j];
						if((unmapped.PixY[j])%2 == 0) mapped.PixX[j] =2* unmapped.PixX[j]+1;
					}
				 /* }
					if((int)((unmapped.PixY[j]-minY)/2)%2==0){
						if((unmapped.PixY[j])%2 != 0) mapped.PixX[j] =2* unmapped.PixX[j];
						if((unmapped.PixY[j])%2 == 0) mapped.PixX[j] =2* unmapped.PixX[j]+1;
					}*/
					mapping_old->SetBinContent(int(unmapped.PixX[j]+1),int(unmapped.PixY[j]-minY+1),(int)(unmapped.PixY[j]-minY)*6+(int)unmapped.PixX[j]+1);
				//	mapping_new->SetBinContent(int(mapped.PixX[j]+1),int(mapped.PixY[j]-(int)minY/2+1),(int)(mapped.PixY[j]-(int)minY/2)*12+(int)mapped.PixX[j]+1);
					mapping_new->SetBinContent(int(mapped.PixX[j]+1),int(mapped.PixY[j]-(int)(minY/2)+1),(int)(unmapped.PixY[j]-minY)*6+(int)unmapped.PixX[j]+1);
					mapping_corX->Fill(int(unmapped.PixX[j]),int(mapped.PixX[j]));
					mapping_corY->Fill(int(unmapped.PixY[j]-minY),int(mapped.PixY[j])-(int)minY/2);
				//	cout <<" Entry " << i << " |X= " << unmapped.PixX[j] << " --> " << mapped.PixX[j]<<" |Y= " << unmapped.PixY[j] << " --> " << mapped.PixY[j] << endl;
				//	cout <<" Entry " << minY << " |X= " << int(unmapped.PixX[j]) << " --> " << mapped.PixX[j]<<" |Y= " << int(unmapped.PixY[j]-minY) << " --> " << mapped.PixY[j] << endl;
					}
				}
				m_pltree->Fill();
			}//end loop over all entries

		//
		// write trees and maps
		//
		fnew->cd();
		dnew->cd();
		m_pltree->Write();
		intercepts->Write();
		
		fmap->cd();
		dmap->cd();
		mapping_old->Write();
		mapping_new->Write();
		mapping_corX->Write();
		mapping_corY->Write();
	}//end loop over all DUT planes
	
	TTree *tsumOut = tsumIn!=0 ? tsumIn->CloneTree() : NULL;
	TTree *tEventOut = tEventIn!=0 ? tEventIn->CloneTree() : NULL;
	if(tsumOut!=0){ fnew->cd(); tsumOut->Write(); }
	if(tEventOut!=0){ fnew->cd(); tEventOut->Write(); }
	fnew->Print();
	fnew->Close();
	fmap->Close(); 
	cout << "Mapping done." << endl;

	//
	// close input file
	// 
	f->Close();	
	
	return 42; // ??? why
}

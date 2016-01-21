//THIS MACRO IS USED ONLY FOR CCPD Version 4

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

int mapping(char *input, char *output) {
  gROOT->Reset();

  //
  // open input file and get TTrees
  //
  TFile *f = TFile::Open(input);
  if(f->IsZombie()){
    cout << "Cannot open '" << input << "' for reading. Exit." << endl;
    return 1;
  }
  TTree *t = (TTree*)f->Get("Plane0/Hits");
  TTree *tsumIn = (TTree*)f->Get("SummaryTree");
  TTree *tEventIn = (TTree*)f->Get("Event");  
  if(!tsumIn) cout << "WARNING: no SummaryTree found in input file..." << endl; 
  if(!tEventIn) cout << "WARNING: no EventTree found in input file..." << endl; 
  
  //
  // open output file
  //
  TFile *fnew = new TFile(output, "RECREATE");
  if(fnew->IsZombie()){
    cout << "Cannot open '" << output << "' for writting. Exit." << endl;
    return 2;
  }
  TDirectory *dnew = fnew->mkdir("Plane0");
  dnew->cd();
  TTree *m_pltree = new TTree("Hits", "Hits");
  TTree *tsumOut = tsumIn!=0 ? tsumIn->CloneTree() : NULL;
  TTree *tEventOut = tEventIn!=0 ? tEventIn->CloneTree() : NULL;

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
  m_pltree->Branch("PixX", merged.PixX, "HitPixX[NHits]/I");
  m_pltree->Branch("PixY", merged.PixY, "HitPixY[NHits]/I"); 
  m_pltree->Branch("Value", unmerged.Value, "HitValue[NHits]/I"); // same as input
  m_pltree->Branch("Timing", unmerged.Timing, "HitTiming[NHits]/I"); // same as input
  m_pltree->Branch("HitInCluster", unmerged.HitInCluster, "HitInCluster[NHits]/I"); // same as input
  m_pltree->Branch("PosX", unmerged.PosX, "HitPosX[NHits]/D"); // same as input
  m_pltree->Branch("PosY", unmerged.PosY, "HitPosY[NHits]/D"); // same as input
  m_pltree->Branch("PosZ", unmerged.PosZ, "HitPosZ[NHits]/D"); // same as input
    
  //  int minY = std::numeric_limits<int>::max();
  int minY = 1000000; // allow to run macro non-compiled
  //cout<<"Initialising minY "<<minY<<endl;




  int nentries = t->GetEntries();
  for (int i=0; i<nentries; ++i) {
    t->GetEntry(i);
    for (int j=0; j<unmerged.NHits; ++j) {
      if(unmerged.PixY[j]==0)continue;
      if(unmerged.PixY[j] < minY) minY = unmerged.PixY[j];
    }
  } //cout<<"Minimum PixY "<<minY<<endl;
  
  TH2D *mapping_old=new TH2D("mapping_old","mapping_old",6,-0.5,6.5,16,-0.5,15.5); 
  TH2D *mapping_corX=new TH2D("mapping_corX","mapping_corX",6,-0.5,5.5,12,-0.5,11.5); 
  TH2D *mapping_corY=new TH2D("mapping_corY","mapping_corY",16,-0.5,15.5,8,-0.5,7.5); 
  TH2D *mapping_new=new TH2D("mapping_new","mapping_new",12,-0.5,11.5,8,-0.5,7.5);
 
  for (int i=0; i<nentries; ++i) { //loop over entries
    t->GetEntry(i);
    for (int j=0; j<unmerged.NHits; ++j) { //loop over NHits
      if ( unmerged.NHits > 0 ) {
	if (minY%2 == 0) merged.PixY[j] = TMath::FloorNint((unmerged.PixY[j]/*+1*/)/2.0 );
	else merged.PixY[j] = TMath::FloorNint( (unmerged.PixY[j]-1.0)/2.0 );
       // if((int)((unmerged.PixY[j]-minY)/2)%2!=0){
        if((int)((unmerged.PixX[j]))%2!=0){
        if((unmerged.PixY[j])%2 != 0) merged.PixX[j] =2* unmerged.PixX[j]+1;
        if((unmerged.PixY[j])%2 == 0) merged.PixX[j] =2* unmerged.PixX[j];
         }

        if((int)((unmerged.PixX[j]))%2==0){
        if((unmerged.PixY[j])%2 != 0) merged.PixX[j] =2* unmerged.PixX[j];
        if((unmerged.PixY[j])%2 == 0) merged.PixX[j] =2* unmerged.PixX[j]+1;
        }
       /* }
        if((int)((unmerged.PixY[j]-minY)/2)%2==0){
        if((unmerged.PixY[j])%2 != 0) merged.PixX[j] =2* unmerged.PixX[j];
        if((unmerged.PixY[j])%2 == 0) merged.PixX[j] =2* unmerged.PixX[j]+1;
        }*/
        mapping_old->SetBinContent(int(unmerged.PixX[j]+1),int(unmerged.PixY[j]-minY+1),(int)(unmerged.PixY[j]-minY)*6+(int)unmerged.PixX[j]+1);
      //  mapping_new->SetBinContent(int(merged.PixX[j]+1),int(merged.PixY[j]-(int)minY/2+1),(int)(merged.PixY[j]-(int)minY/2)*12+(int)merged.PixX[j]+1);
        mapping_new->SetBinContent(int(merged.PixX[j]+1),int(merged.PixY[j]-(int)(minY/2)+1),(int)(unmerged.PixY[j]-minY)*6+(int)unmerged.PixX[j]+1);
        mapping_corX->Fill(int(unmerged.PixX[j]),int(merged.PixX[j]));
        mapping_corY->Fill(int(unmerged.PixY[j]-minY),int(merged.PixY[j])-(int)minY/2);
//	cout <<" Entry " << i << " |X= " << unmerged.PixX[j] << " --> " << merged.PixX[j]<<" |Y= " << unmerged.PixY[j] << " --> " << merged.PixY[j] << endl;
//	cout <<" Entry " << minY << " |X= " << int(unmerged.PixX[j]) << " --> " << merged.PixX[j]<<" |Y= " << int(unmerged.PixY[j]-minY) << " --> " << merged.PixY[j] << endl;
      }      
    }    
    m_pltree->Fill();
  }

 TFile *fmap = new TFile("mapping.root","RECREATE");
 mapping_old->Write();
 mapping_new->Write();
 mapping_corX->Write();
 mapping_corY->Write();
 fmap->Close(); 
  //
  // write trees and close output file
  //
  fnew->cd();
  dnew->cd();
  m_pltree->Write();
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

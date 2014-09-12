void averageEfficiency(char *name)
{
  TFile *f = new TFile(name);
  cout << "Input file =  " << name << endl;
  TEfficiency *eff = (TEfficiency*)f->Get("Efficiency/DUTPlane0MapC");

  TH2F *box = new TH2F("kip", NULL, 2,-3000, 0, 2, 0, 2500);
  box->Draw();
  eff->Draw("samecolz");
  
  std::cout<< "Stat option = "<< eff->GetStatisticOption()<<std::endl;
  eff->SetStatisticOption(0);
  
  //Getting the passed and total histograms
  
  TH1* hpassed = (TH1*) eff->GetPassedHistogram();
  hpassed->SetDirectory(0);
  
  TH1* htotal = (TH1*) eff->GetTotalHistogram();
  htotal->SetDirectory(0);

  //Para calcular average quantities 
  double eInd=0., tot=0., av = 0., eu=0, ed=0;
  
  //Para calcular global error;
  double eGlob=0., totalEntries = 0.;
  
  for(int bx=1; bx<=30; ++bx){  
  //for(int bx=3; bx<=4; ++bx){  
    for(int by=76; by<=87; ++by){ // start at normal pixels
      if(bx==3 && by==78) continue; // not account for dead pixel
      if(bx==1 || by==78) continue; // exclude edges for effResults_cosmic_2163-2169.root
      int bin = eff->GetGlobalBin(bx,by);
      double x =  eff->GetEfficiency(bin);
      if(x!=0){	
	av += x;
	
	//std::cout << " Total = "<< htotal->GetBinContent(bx, by) <<std::endl;
	
	
	double errorInd = (1/ htotal->GetBinContent(bx, by))* sqrt( x*(1- x/ htotal->GetBinContent(bx, by) ) );  	
	eInd += errorInd;
	
	totalEntries += htotal->GetBinContent(bx, by);
	
	eu += eff->GetEfficiencyErrorUp(bin);
	ed += eff->GetEfficiencyErrorLow(bin);
	tot += 1;
	//cout << bx << " " << by << " " << setprecision(8) << x << endl;
      }
    }
  }
  if(tot!=0){
    av /= tot;
    eInd /= tot;
    eu /= tot;
    ed /= tot;
  }
  
  if (totalEntries!=0){
    eGlob= (1/ totalEntries )*TMath::Sqrt( av*(1- av/ totalEntries ) );
  
  }
  
  cout << endl 
       << "Average efficiency = " << setprecision(3) <<100.*av 
       << " + " << setprecision(2) << 100*eu 
       << " - " << setprecision(2) << 100*ed 
       <<  "  [%] " << endl << endl;
       
       
  cout << endl 
       << "Binomial individual efficiency = " << setprecision(3) <<100.*av 
       << " + " << setprecision(2) << 100*eInd 
       <<  "  [%] " << endl << endl;     

  cout << endl 
       << "Binomial global efficiency = " << setprecision(3) <<100.*av 
       << " + " << setprecision(2) << 100*eGlob 
       <<  "  [%] " << endl << endl;     
 

  f->Close();
   
}

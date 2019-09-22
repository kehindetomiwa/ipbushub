/**
 * Measure the rate of the ipbus communication
 * Author: kehinde tomiwa
 *
 */
#include <iostream>
#include <ctime>
#include <sys/time.h> 
#include <vector>
#include "ipbus/Uhal.h"
#include <TFile.h>
#include <TTree.h>
#include <unistd.h>

using namespace std;
using namespace ipbus;

int main(int argc, const char* argv[]){

  if(argc<3){
    cout << "Usage: " << argv[0] << " <host> <addr> <smps> <evts> <port> <output-file>" << endl;
    return 0;
  }

  const char * host = argv[1];
  uint32_t addr = strtoul(argv[2],NULL,16);
  uint32_t smps = strtoul(argv[3],NULL,10);
  uint32_t evts = strtoul(argv[4],NULL,10);
  uint32_t port = atoi(argv[5]);   
  const char  *hh = (argc<7?"performance.root":argv[6]);
  cout << "Measure the rate on"
       << " host: " << host 
       << " addr: " << addr 
       << " smps: " << smps 
       << " evts: " << evts
	   << " port: " << port
       << endl;

  cout << "Create ntuple" << endl;
  TFile * tfile = new TFile( hh,"RECREATE"); 
  TTree * tuple = new TTree("ipbus","IPBus performance tree");
  uint32_t psize = 0;
  double   ptime = 0.;
  tuple->Branch("size",&psize,"PackageSize/I");
  tuple->Branch("time",&ptime,"ReadoutTime/D");

  cout << "Connect to server" << endl;
  Uhal::SetVerbose(true);
  Uhal * client = new Uhal(host, port);
  vector<uint32_t> v;
  timeval tim;  
  gettimeofday(&tim, NULL);  
  double t0,t1,t2;
  t0=tim.tv_sec+(tim.tv_usec/1000000.0);  
  cout << "Do loop" << endl;  
  for(psize=1;psize<=smps;psize++){
	for(uint32_t evt=0;evt<evts;evt++){
	  gettimeofday(&tim, NULL);  
	  t1=tim.tv_sec+(tim.tv_usec/1000000.0);  
	  client->Read(addr, v, psize);
	  if(v.size()==0){
		while(!client->IsSynced()){
		  client->Sync();
		}
	  }
	  //for(uint32_t i=0;i<psize*10000;i++){}
	  gettimeofday(&tim, NULL);  
	  t2=tim.tv_sec+(tim.tv_usec/1000000.0);  
	  ptime = t2-t1;
	  tuple->Fill();
	}
  }
  
  float seconds = t2-t0;
  cout << "Elapsed time (seconds): " << seconds << endl;
  cout << "Event rate (Hz): " << float(evts)/float(seconds) << endl;  

  cout << "Close ntuple and exit" << endl;
  tuple->Write();
  tfile->Close();
  delete tfile;
  delete client;
  
  return 0;
}

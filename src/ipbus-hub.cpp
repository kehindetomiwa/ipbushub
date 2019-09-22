/*
  Author:Kehinde Tomiwa
*/

#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "ipbus/PacketBuilder.h"
#include "ipbus/Packet.h"
#include "ipbus/Uhal.h"
#include "ipbus/PacketHeader.h"
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <ifaddrs.h>
#include <vector>
using namespace std;
using namespace ipbus;

//Global variables
std::map<uint32_t,uint32_t> * g_expected_pkid;
std::mutex * g_lock;
Uhal * g_server;
bool g_verbose = false;
bool g_dirty = true;
uint32_t memSizePerClient = 1000;

//Forward declarations
void Listen(uint32_t );

//Main function
int main (int argc, char **argv){
 
  if(argc<3){
    cout << "Usage: " << argv[0] << " <server_ipaddress> [-p ports] [-v]" << endl
		 << " p ports : list of ports to listen on" << endl
		 << " v       : enable verbose mode" << endl;
    return 0;
  }

  vector<uint32_t> client_ports;
  bool verbose = false;

  for(int i=1;i<argc;i++){
	const char * pos=strstr(argv[i],"-p");
	if(pos==NULL) continue;
	if(strlen(argv[i])>2){
	  client_ports.push_back(atoi(&argv[i][2]));
	  for(int j=i+1;j<argc;j++){
		pos=strstr(argv[j],"-");
		if(pos!=NULL){break;}
		client_ports.push_back(atoi(argv[j]));
	  }
	  argc=argc-client_ports.size();
	  for(int j=i;j<argc;j++){
		argv[j]=argv[j+client_ports.size()];
	  }
	}else{
	  client_ports.push_back(atoi(argv[i+1]));
	  for(int j=i+2;j<argc;j++){
		pos=strstr(argv[j],"-");
		if(pos!=NULL){break;}
		client_ports.push_back(atoi(argv[j]));
	  }
	  argc=argc-client_ports.size();
	  for(int j=i;j<argc;j++){
		argv[j]=argv[j+client_ports.size()];
	  }
	}
  }

  for(int i=1;i<argc;i++){
	const char * pos=strstr(argv[i],"-v");
	if(pos==NULL) continue;
	verbose=true;
	for(int j=i;j<argc;j++){
	  argv[j]=argv[j+1];
	}
  }


  //Parse arguments
  //server
  string server_name = string(argv[1]);
   
  //Now initialize Uhal

  Uhal::SetVerbose(verbose);
  g_verbose = verbose;

  cout << "Connect to server " << server_name << endl;
  g_server = new Uhal(server_name, 50001);
  
  //create expected pkid map
  g_expected_pkid = new std::map<uint32_t,uint32_t>();
  for(uint32_t i=0;i<client_ports.size();i++){
	g_expected_pkid->insert(std::pair<uint32_t,uint32_t>(client_ports.at(i),0) );
  }
  
  //create lock
  g_lock = new std::mutex();
  
  //create threads
  vector<thread*> threads;
  for(uint32_t i=0;i<client_ports.size();i++){
	cout << "Create Listener in port " << client_ports.at(i) << endl;
	threads.push_back(new thread(Listen,client_ports.at(i)));
  }

  // Show the host ip address
  struct ifaddrs *ifaddr, *ifa;
  struct sockaddr_in *hn;
  char *addr;
  vector<string> addresses;
  
  cout << "Hub is running with " << client_ports.size() << " listeners" << endl
	   << " ports: ";
  for(uint32_t i=0;i<client_ports.size();i++){
	cout << client_ports.at(i) << " ";
  }
  cout << endl
	   << "Available connection addresses" << endl;
  
  getifaddrs(&ifaddr);
  for (ifa = ifaddr; ifa; ifa = ifa->ifa_next){
    if (ifa->ifa_addr->sa_family == AF_INET){
      hn = (struct sockaddr_in *)ifa->ifa_addr;
      addr = inet_ntoa(hn->sin_addr);
      cout << " " << ifa->ifa_name << " " << addr << endl;
	}
  }
  freeifaddrs(ifaddr);

  //Wait for the threads to finish
  for(uint32_t i=0;i<client_ports.size();i++){
	threads.at(i)->join();
  }

  cout<<"Have a nice day"<<endl;
  delete g_server;

}
// Function Definition
void Listen(uint32_t port){

  //struct sockaddr_in client, host;
  int32_t sock;
  sock = socket(AF_INET,SOCK_DGRAM,0);
  struct sockaddr_in rxaddr;
  //rxaddr.sin_len = sizeof(rxaddr);
  rxaddr.sin_family = AF_INET;
  rxaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  rxaddr.sin_port = htons(port);

  //reuse address, port and linger on close
  int opt=1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));
  #ifdef SO_REUSEPORT
  setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const void*)&opt, sizeof(opt));
  #endif
  struct linger onclose;
  onclose.l_onoff = 0;
  onclose.l_linger = 1;
  setsockopt(sock, SOL_SOCKET, SO_LINGER, (const void*)&onclose, sizeof(onclose));

  //set timeout option 
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;  
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv)); 
  
  //bind socket (Note we use global namespace and not std::bind from c++11)  
  int ret = ::bind(sock,(struct sockaddr *)&rxaddr,sizeof(rxaddr));
  if(ret<0) {cout << "Error binding socket" << endl; return; }

  //memory client pkid to server pkid
  std::map<uint32_t,uint32_t> memPkid;
  std::unordered_map<uint32_t,Transaction*> memData;
  std::unordered_map<uint32_t,Transaction*>::iterator memDataIt;
 
  //Tool to decode packets
  PacketBuilder pb; 
  
  // Nice name
  ostringstream os;
  os << "Listener[" << port << "]"; 
  string lname = os.str();

  cout << lname << " Running" << endl;
  
  // status reply
  Status* status = new Status();
  Resend* resend = new Resend();
 
  status->SetBufferSize(memSizePerClient);
	  
  // Wait until you receive a request from the client	
  while(1){
	
	struct sockaddr_in sender_addr;
	//struct sockaddr_storage sender_addr;
	socklen_t sender_len=sizeof(sender_addr);
	int32_t n=recvfrom(sock,pb.GetBytes(),10000,0,
					   (struct sockaddr *)&sender_addr,&sender_len);
	
	if(n<0){ continue; }
	  
	os.str("");
	os << ((ntohl(sender_addr.sin_addr.s_addr) & 0xFF000000)>>24) << "." 
	   << ((ntohl(sender_addr.sin_addr.s_addr) & 0x00FF0000)>>16) << "." 
	   << ((ntohl(sender_addr.sin_addr.s_addr) & 0x0000FF00)>>8 ) << "." 
	   << ((ntohl(sender_addr.sin_addr.s_addr) & 0x000000FF)>>0 ) << "." 
	   << ":" << ntohs(sender_addr.sin_port);
	
	string cname(os.str());
	
	cout << lname << " Packet received from " << cname << endl;

	if(!g_server->IsSynced()){
	  g_lock->lock();
	  g_server->Sync();
	  g_lock->unlock();
	  if(!g_server->IsSynced()){
		cout << lname << " Server is not synced. Drop request. ";
		continue;
	  }
	}

	//unpack
	pb.SetLength(n);
	pb.Unpack();
	
	PacketHeader header = pb.GetHeader();
	//if(g_verbose) header.Dump();

	
	if(header.GetType()==header.STATUS){
	  
	  cout << lname << " Received Status request from " << cname << endl;

	  //Tell him what is the expected pkid we want from him
	  status->SetNextExpectedPacketId(g_expected_pkid->at(port)); 
	  status->Pack();
	  cout << lname << " Send Status reply to " << cname << endl;
	  if(g_verbose) status->Dump();
	  sendto(sock,status->GetBytes(),   
			 status->GetLength(),0,
			 (struct sockaddr *)&sender_addr,
			 sizeof(sender_addr));
	  
	}else if(header.GetType()==header.CONTROL){
	   
	  cout << lname << " Received Transaction request from " << cname << endl;
	  
	  Packet * req = (Packet *) pb.GetPacket();	 
	  // get the packet id and store as client packet id
	  uint32_t client_pkid = req->GetPkid();
	  cout << lname 
		   << " ExpPkid: " << hex << g_expected_pkid->at(port) << dec
		   << " RcvPkid: " << hex << client_pkid << dec
		   << " " << cname << endl;
	
	  //check if received pkid is expected pkid
	  if(client_pkid==g_expected_pkid->at(port)){		

		//lock to communicate with server
		g_lock->lock();

		//Unpack the request
		req->Unpack();
		//Change by the expected pkid by server
		uint32_t server_pkid = Packet::GetNextPacketId();
		req->SetPkid(server_pkid);
		
		//keep track of it
		memPkid[client_pkid]=server_pkid;

		// forward the packet to the server  
		cout << lname << " Forward Transaction to server with Pkid: " << hex << req->GetPkid() << dec << endl;
		
		//send packet to server
		Packet * reply = (Packet *) g_server->Send(req);
		
		//unlock after communication with server
		g_lock->unlock();
		 
		if(reply==NULL){
		  cout << lname << " Server reply timed out. Sync and Drop request. ";
		  g_server->Sync();
		  continue;
		}
		
		cout << lname << " Received reply form server." << endl;
		
		//Change the pkid back to the original request
		reply->SetPkid(client_pkid);
		cout << lname << " Send transaction reply to " << cname << endl;	     	     
		// send back packet to client 
		
		sendto(sock,reply->GetBytes(),
			   reply->GetLength(),0,
			   (struct sockaddr *)&sender_addr,
			   sizeof(sender_addr));
		
		//keep a copy
		Transaction * copy = new Transaction();
		copy->CopyBytes(reply->GetBytes(),reply->GetLength());
		copy->Unpack();
		memData[client_pkid]=copy;
		memDataIt=memData.begin();
		while(memData.size()>memSizePerClient){
		  delete memDataIt->second;
		  memDataIt=memData.erase(memDataIt);
		}
	
		//Increase the expected pkid for this listener
		g_expected_pkid->at(port)++;
		if(g_expected_pkid->at(port)>0xFFFF) g_expected_pkid->at(port)=0;
				
	  }else{
		cout << lname << " Drop request from " << cname << endl;
		continue;
	  }
	  
	}else if(header.GetType()==header.RESEND){

	  cout << lname << " Received Resend request from " << cname << endl;
	  
	  Packet * req = (Packet *) pb.GetPacket();
	  //get the client pkid
	  uint32_t client_pkid = req->GetPkid();
	  
	  Packet * reply = NULL;
	  //check if it is still in the hub
	  memDataIt=memData.find(client_pkid);
	  if(memDataIt!=memData.end()){
		cout << lname << " Send reply from memory size " << memData.size() << endl;
		reply=memDataIt->second;
	  }else{
		
		//match the server pkid
		uint32_t lost_pkid = memPkid[client_pkid];
	  
		//if(p->GetNextExpectedPacketId()>req->GetPkid()+p->GetBufferSize()) return NULL;
		
		//change the request pkid
		req->SetPkid(lost_pkid);
		
		//forward to server
		g_lock->lock();
		reply = (Packet *) g_server->Send(req);
		g_lock->unlock();

		if(reply==NULL){
		  cout << lname << " Server reply timed out. Sync and Drop request. ";
		  g_server->Sync();
		  continue;
		}
		reply->SetPkid(client_pkid);
	  }
  	  
	  // send back packet to client 
	  sendto(sock,reply->GetBytes(),
			 reply->GetLength(),0,
			 (struct sockaddr *)&sender_addr,
			 sizeof(sender_addr)); 
	}
  }
  
  memDataIt=memData.begin();
  while(memData.size()>memSizePerClient){
	delete memDataIt->second;
	memDataIt=memData.erase(memDataIt);
  }
  delete status;
  delete resend;
}

#include <iostream>
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

using namespace std;
using namespace ipbus;

int main (int argc, char **argv){
  
  if(argc<2)
  {
    cout << argv[0] << " <server> " << endl;
    return -1;
  }
  
  //Parse arguments
  //server
  string server_name = string(argv[1]);
  
  //Create a client listener
  //struct sockaddr_in client, host;
  int32_t sock;
  uint32_t port=50001;
  uint32_t port2 = 50002;

  //Tool to decode packets
  PacketBuilder pb; 

  
  
  /*
	memcpy(&server.sin_addr, serverhost->h_addr_list[0], serverhost->h_length);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
  */
  
  //Create a server writer
  Uhal * server = new Uhal(server_name, port);
  bool enabled = true;
  server->SetVerbose(enabled);
  std::map<uint32_t,uint32_t> expected_pkid;
  expected_pkid[50002]=0;
   
  // Hub Listening on the a udp socket
  sock = socket(AF_INET,SOCK_DGRAM,0);
  struct sockaddr_in rxaddr;
  rxaddr.sin_family = AF_INET;
  rxaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  rxaddr.sin_port = htons(port2);
  
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  
  //set timeout option 
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv)); 
  
  //  bind socket   
  bind(sock,(struct sockaddr *)&rxaddr,sizeof(rxaddr));

  // Wait until you receive a request from the client	
  cout<<"Waiting for Client request"<<endl;
  while(1){  
	struct sockaddr_in sender_addr;
	socklen_t sender_len=sizeof(sender_addr);
	 int32_t n=recvfrom(sock,pb.GetBytes(),10000,0,
					   (struct sockaddr *)&sender_addr,&sender_len);
	 if(n<0){ continue; }
	 cout<<"packet received....."<<endl;
	 //check which port receive it......
	 
	 //unpack
	 pb.SetLength(n);
	 pb.Unpack();
	 
	 PacketHeader header = pb.GetHeader();
	 header.Dump();

	 if(header.GetType()==header.STATUS){
	   cout<<"Status Packtet received...."<<endl;
	   Status* reply = new Status(); 
	   reply->SetNextExpectedPacketId(expected_pkid[50002]); 
	   reply->Pack();
	   sendto(sock,reply->GetBytes(),   
			  reply->GetLength(),0,
			  (struct sockaddr *)&sender_addr,
			  sizeof(sender_addr));
	   cout<<"Status sent...."<<endl;
	   delete reply;
	 }else if(header.GetType()==header.CONTROL){
	   cout<<"Transaction Packet received..."<<endl;
	   Packet * req = (Packet *) pb.GetPacket();	 
	   
	   // get the packet id and store as client packet id
	   uint32_t client_pkid = req->GetPkid();
	   cout<<"Client_pkid 1.\t"<<client_pkid<<endl;//client pkid
	   
	   //check if expected packet id
	   if(client_pkid==expected_pkid[50002]){
		 req->SetPkid(Packet::GetNextPacketId());
	     req->Pack();
		 
		 // forward the packet to the server  
		 cout<<"sending packet to server ....."<<endl;
		 Packet * reply = (Packet *) server->Send(req);
		 reply->Unpack();
		 reply->SetPkid(expected_pkid[50002]);
		 reply->Pack();
		 cout<<"Reply from Server:"<<endl;
		 reply->Dump();
	     
	     
		 // send back packet to client 
		 sendto(sock,reply->GetBytes(),
				reply->GetLength(),0,
				(struct sockaddr *)&sender_addr,
				sizeof(sender_addr));
		 cout<<"Request Sent back to Client"<<endl;
		 cout<<"expected_pkid[50002]\t"<<expected_pkid[50002]<<endl;
		 cout<<"Client_pkid 2. \t"<<client_pkid<<endl;//client pkid
		 
		 expected_pkid[50002]++;
		 if(expected_pkid[50002]>0xFFFF) expected_pkid[50002]=0;
				
	   }else{
		 cout<<"drop client"<<client_pkid<<endl;
		 cout<<"drop expected_pkid[50002]\t"<<expected_pkid[50002]<<endl;
		 cout<<"drop Client_pkid 2.\t"<<client_pkid<<endl;//client pkid
		 cout<<"Packet dropped.."<<endl;
		 continue;
	   }
	   cout<<"Waiting for Client request again"<<endl;
	 }else if(header.GetType()==header.RESEND){
	   cout<<"Resending"<<endl;
	 }
  }
  
  delete server;
  return 0;
}


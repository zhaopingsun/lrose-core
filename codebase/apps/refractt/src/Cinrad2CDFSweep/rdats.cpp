#include "rdats.h"
#include<vector>
#include <stdio.h>
#include <string.h>
size_t getSweepLength(TSSweepHeader *swphdr)
{
	int chan=swphdr->chan;
	if(chan==0) chan=1;
	
	return sizeof(*swphdr)+chan*swphdr->binnum*2*sizeof(float)+swphdr->burstbinnum*2*sizeof(float);
}
TSSweepHeader *getNextSwpHeader(TSSweepHeader *swphdr)
{
	int swplen=getSweepLength(swphdr);
	return (TSSweepHeader*) ((char*)swphdr+swplen);
}
Iqcmpl *getIQData(TSSweepHeader *swphdr)
{
	return (Iqcmpl*)((char*)swphdr+sizeof(TSSweepHeader));
}
vector<char> fileCache;
int scanIQFile(const char*fname,TSHeader *tsh,SwpHdrList &swplist)
{
	int const IQ_START_POS=sizeof(TSHeader)+256;
	FILE *fp=fopen(fname,"rb");
	if(fp==NULL)
		return 0;
	fileCache.resize(IQ_START_POS);
	fread(&fileCache[0],1,sizeof(TSHeader),fp);
	char *pbFile=&fileCache[0];
	TSHeader *IQFileHead=(TSHeader *)pbFile; 

	fseek(fp,0,SEEK_END);
    	int fileLength=ftell(fp);

	if(IQFileHead->version>=TS_VER_IQ16)
	{
		vector<char> tmpCache;
		tmpCache.resize(fileLength);
		fread(&tmpCache[0],1,fileLength,fp);
		
		fileLength=;
		//decode sweep here
		
	}
	else
	{
		rewind(fp);
		fread(&fileCache[0],1,fileLength,fp);
		memcpy(tsh,IQFileHead,sizeof(*tsh));

	}


	TSSweepHeader *SWFileHead =(TSSweepHeader*)(pbFile+IQ_START_POS);
	TSSweepHeader *nexSWFileHead;
	nexSWFileHead=SWFileHead;

	size_t totalen=fileLength;
	 
	size_t datalen;
	datalen=IQ_START_POS;

	while(datalen<totalen)
	{	
		swplist.push_back(nexSWFileHead);
		size_t swpLEN=getSweepLength(nexSWFileHead);	 
		nexSWFileHead=getNextSwpHeader(nexSWFileHead);
		datalen=datalen+swpLEN;	
	}
	return 1;
}

#ifdef UNICODE
	#undef UNICODE
#endif

#include "DataChannel.h"
#include<stdio.h>
DataChannel::DataChannel(void)
{
   //count = 0; 
}

DataChannel::~DataChannel(void)
{
}



std::vector<sEMGdata> DataChannel::GetData(int ChannelId)
{
    //char str[100];
	std::vector<sEMGdata> retData;
	sEMGdata* bufferData = new sEMGdata[10];
	int result = get_sEMG_data(ChannelId,10,bufferData);
    //sprintf(str, "result = :%d\n", result);
	//OutputDebugString(str);
	if(result == 0)
	{
		//delete[] bufferData;
		//OutputDebugString(TEXT("result == 0\n") );
	}
	else
	{
		for(int i=0;i<result;i++)
		{
			retData.push_back(bufferData[i]);
		}
	}
	delete[] bufferData;
	return retData;
}
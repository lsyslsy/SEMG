#pragma once
#include <windows.h>
#include <vector>
#include "../sEMG_DLL/socket.h"
#include "../sEMG_DLL/sEMG_DLL.h"
class DataChannel
{
public:
	DataChannel(void);
	~DataChannel(void);

    //int count;
    //std::vector<sEMGdata> retData;
	std::vector<sEMGdata> GetData(int ChannelId);
};
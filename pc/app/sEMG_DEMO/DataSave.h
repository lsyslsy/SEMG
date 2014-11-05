#pragma once
#include <windows.h>
#include <map>
#include "Data.h"
#include "../sEMG_DLL/socket.h"
#include "../sEMG_DLL/sEMG_DLL.h"
class DataSave
{
public:
	DataSave(void);
	~DataSave(void);

    std::map<int,Data> Channels;
};
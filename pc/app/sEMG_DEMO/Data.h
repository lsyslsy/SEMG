#pragma once
#include <vector>
#include <map>
#include "../sEMG_DLL/socket.h"
#include "../sEMG_DLL/sEMG_DLL.h"

class Data
{
public:
	Data(void);
	~Data(void);

    //int count;
   std::vector<sEMGdata> sData;
};
#include "Butterworth.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "semg_debug.h"
using namespace std;

const double Butterworth::NUM[5] = {
	0.9823854385261, -3.737511389482, 5.519636115962, -3.737511389482,
	0.9823854385261 };
const double Butterworth::DEN[5] = {
	1, -3.770723788898, 5.519325819115, -3.704298990066,
	0.9650811738991 };

Butterworth::Butterworth()
{
	
}


Butterworth::~Butterworth()
{
}

void Butterworth::filter_init(Butterworth_4order *channel)
{
	channel->x0 = 0;
	channel->x1 = 0;
	channel->x2 = 0;
	channel->x3 = 0;
	channel->x4 = 0;
	channel->y0 = 0;
	channel->y1 = 0;
	channel->y2 = 0;
	channel->y3 = 0;
	channel->y4 = 0;
}

/**
* @brief  the filter use to filter the signal off 50Hz interpret.
* @param [in] DIN: a pointer  to the start address of input buffer
* @param [out] DOUT: a pointer to the start address of output buffer
* @param [in] LEN: the Length of DIN and DOUT
* @return: void
* @see:
* @note:
*/

void Butterworth::filter(Butterworth_4order *channel, double *DIN, double *DOUT, unsigned int LEN)
{
	double tmp;
	unsigned int i;
	for (i = 0; i<LEN; i++)
	{

		channel->x0 = *DIN;
		tmp = NUM[0] * channel->x0 + NUM[1] * channel->x1 + NUM[2] * channel->x2 + \
			NUM[3] * channel->x3 + NUM[4] * channel->x4 - DEN[1] * channel->y1 \
			- DEN[2] * channel->y2 - DEN[3] * channel->y3 - DEN[4] * channel->y4;
		tmp /= DEN[0];
		if (tmp >= DATASCALE)
		{
			tmp = DATASCALE;
		}
		if (tmp <= -DATASCALE)
		{
			tmp = -DATASCALE;
		}

		*DOUT = tmp;

		channel->x4 = channel->x3; channel->x3 = channel->x2; channel->x2 = channel->x1; channel->x1 = channel->x0;
		channel->y4 = channel->y3; channel->y3 = channel->y2; channel->y2 = channel->y1; channel->y1 = *DOUT;

		DIN++;
		DOUT++;


	}
}
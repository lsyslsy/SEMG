#include "RLS.h"
#include <cmath>

#include "semg_debug.h"

# define M_PI       3.14159265358979323846  /* pi */
const double RLS::fb = 50;
const double RLS::fs = 1000;//1000HZ
const double RLS::lambda = 0.996;
const double RLS::ssin[20] = { 0.000000, 0.309017, 0.587785, 0.809017, 0.951057, 1.000000, 0.951057, 0.809017, 0.587785, 0.309017, 0.000000, -0.309017, -0.587785, -0.809017, -0.951057, -1.000000, -0.951057, -0.809017, -0.587785, -0.309017 };
RLS::RLS()
{
}


RLS::~RLS()
{
}
void RLS::filter_init(RLS_PARM *channel)
{
	channel->b = 0;
	channel->c = 0;
	channel->e = 0;
	channel->r1 = 1;
	channel->r4 = 1;
}

/**
* @brief: the filter use to filter the signal off 50Hz interpret
* @param [in] DIN: a pointer  to the start address of input buffer
* @param [out] DOUT: a pointer to the start address of output buffer
* @param [in] LEN: the Length of DIN and DOUT
* @return: void
* @see:
* @note:
*/

void RLS::filter(RLS_PARM *channel, double *DIN, double *DOUT, unsigned int LEN)
{
	double x_;
	double tmp = 0;
	unsigned int n;
	int i = 0;
	int j = 0;
	//因为LEN是100的倍数
	for (n = 0; n < LEN; n++)
	{
		i = n % 20;
		j = (n + 5) % 20;
		x_ = channel->b*ssin[i] + channel->c*ssin[j];
		channel->e = DIN[n] - x_;
		channel->r1 = lambda * channel->r1 + ssin[i] * ssin[i];
		channel->r4 = lambda * channel->r4 + ssin[j] * ssin[j];
		channel->b = channel->b + channel->e*ssin[i] / channel->r1;
		channel->c = channel->c + channel->e*ssin[j] / channel->r4;
		tmp = channel->e;
		if (tmp >= DATASCALE)
		{
			tmp = DATASCALE;
		}
		if (tmp <= -DATASCALE)
		{
			tmp = -DATASCALE;
		}
		DOUT[n] = tmp;
		
	}
	
}
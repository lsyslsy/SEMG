#include "Baseline_corrector.h"


Baseline_corrector::Baseline_corrector()
{
}


Baseline_corrector::~Baseline_corrector()
{
}

void Baseline_corrector::correct(double *last, double *DIN, double *DOUT, unsigned int LEN)
{
	double now = 0;
	for (unsigned int i = 0; i < LEN; i++)
	{
		now += DIN[i];
	}
	now /= LEN;
	now = *last * 0.8 + now*0.2;//may be exists iteration 
	for (unsigned int i = 0; i < LEN; i++)
	{
		DOUT[i] = DIN[i] - now;
	}
	*last = now;

}
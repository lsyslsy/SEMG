#include "FOURIER.h"
#include <cmath>

#include "semg_debug.h"

//# define M_PI       3.14159265358979323846  /* pi */
const double FOURIER::fb = 50;
const double FOURIER::fs = 1000;//1000HZ
const double FOURIER::ssin[20] = { 0.000000, 0.309017, 0.587785, 0.809017, 0.951057, 1.000000, 0.951057, 0.809017, 0.587785, 0.309017, 0.000000, -0.309017, -0.587785, -0.809017, -0.951057, -1.000000, -0.951057, -0.809017, -0.587785, -0.309017 };
FOURIER::FOURIER()
{
}


FOURIER::~FOURIER()
{
}
void FOURIER::filter_init(FOURIER_PARM *channel)
{

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

void FOURIER::filter(FOURIER_PARM *channel, double *DIN, double *DOUT, unsigned int LEN)
{
	double tmp = 0;
	unsigned int n;
	int i = 0;
	int j = 0;
	double S1 = 0;
	double S2 = 0;
	double A = 0;
	double B = 0;
	//因为LEN是100的倍数
	for (n = 0; n < LEN; n++) {
		i = n % 20;
		j = (n + 5) % 20;
		S1 = S1 + DIN[n] * ssin[i];
		S2 = S2 + DIN[n] * ssin[j];

	}

	A = S1 * 2 / 100;
	B = S2 * 2 / 100;
	for (n = 0; n < LEN; n++) {
		i = n % 20;
		j = (n + 5) % 20;
		tmp = A*ssin[i] + B* ssin[j];
		DOUT[n] = DIN[n] - tmp;
		if (DOUT[n] >= DATASCALE) {
			DOUT[n] = DATASCALE;
		}
		if (DOUT[n] <= -DATASCALE) {
			DOUT[n] = -DATASCALE;
		}

	}


}

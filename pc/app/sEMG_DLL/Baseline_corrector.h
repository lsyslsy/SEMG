#pragma once

/**
 *  Baseline drift corrector
 */
class Baseline_corrector
{
public:
	Baseline_corrector();
	static void correct(double *last, double *DIN, double *DOUT, unsigned int LEN);
	~Baseline_corrector();
};


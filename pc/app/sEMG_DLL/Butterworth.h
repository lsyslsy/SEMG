#pragma once
typedef struct Butterworth_4order
{
	//the in signal
	double x0;
	double x1;
	double x2;
	double x3;
	double x4;
	//the out signal
	double y0;
	double y1;
	double y2;
	double y3;
	double y4;

}Butterworth_4order;
class Butterworth
{
public:

	Butterworth();
	static void filter_init(Butterworth_4order *channel);
	static void filter(Butterworth_4order *channel, double *DIN, double *DOUT, unsigned int LEN);
	~Butterworth();
private:
	static  const int NL = 5;
	static  const double NUM[5]; 
	static const int DL = 5;
	static const double DEN[5];
	
};


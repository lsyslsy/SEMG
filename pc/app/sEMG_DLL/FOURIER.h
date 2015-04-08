#pragma once
/**  
 *  @brief A struct type.
 *  FOURIER paramter structure  
 */
typedef struct FOURIER_PARM
{
	//double A;	/**< sin信号系数 */
	//double B;	/**< cos信号系数 */
	//double S1;	/**< 求和保存 */
	//double S2;	/**< 求和保存 */
	

}FOURIER_PARM;

/**
 *  @brief FOURIER类
 *
 *
 */
class FOURIER
{

public:
		FOURIER();

	/**
	 *
	 */
	static void filter_init(FOURIER_PARM *channel);

	static void filter(FOURIER_PARM *channel, double *DIN, double *DOUT, unsigned int LEN);
	~FOURIER();
private :
	static const double fb;
	static const double fs;
	static const double FOURIER::ssin[20];
};


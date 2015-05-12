#pragma once
/**  
 *  @brief A struct type.
 *  FOURIER paramter structure  
 */
typedef struct FOURIER_PARM
{
	//double A;	/**< sin�ź�ϵ�� */
	//double B;	/**< cos�ź�ϵ�� */
	//double S1;	/**< ��ͱ��� */
	//double S2;	/**< ��ͱ��� */
	

}FOURIER_PARM;

/**
 *  @brief FOURIER��
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


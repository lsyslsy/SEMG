#pragma once
/**
 *  @brief A struct type.
 *  RLS paramter structure
 */
typedef struct RLS_PARM
{
	double b;	/**< sin�ź�ϵ�� */
	double c;	/**< cos�ź�ϵ�� */
	double e;	/**< ��� */
	double r1;	/**< �����ϵ��1 */
	double r4;	/**< �����ϵ��2 */

}RLS_PARM;

/**
 *  @brief RLS��
 *
 *
 */
class RLS
{

public:
		RLS();

	/**
	 *
	 */
	static void filter_init(RLS_PARM *channel);

	static void filter(RLS_PARM *channel, double *DIN, double *DOUT, unsigned int LEN);
	~RLS();
private :
	static const double fb;
	static const double fs;
	static const double lambda;
	static const double ssin[20];
};

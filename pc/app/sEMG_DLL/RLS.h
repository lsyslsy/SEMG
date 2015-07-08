#pragma once
/**
 *  @brief A struct type.
 *  RLS paramter structure
 */
typedef struct RLS_PARM
{
	double b;	/**< sin信号系数 */
	double c;	/**< cos信号系数 */
	double e;	/**< 误差 */
	double r1;	/**< 自相关系数1 */
	double r4;	/**< 自相关系数2 */

}RLS_PARM;

/**
 *  @brief RLS类
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

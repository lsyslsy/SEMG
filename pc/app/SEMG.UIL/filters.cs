using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace SEMG.UIL
{
    class filters
    {
         static int NL = 5;
        static double[] NUM = new double[5]{
     0.9823854385261,   -3.737511389482,    5.519636115962,   -3.737511389482,
     0.9823854385261
};
         static int DL = 5;
        static double[] DEN = new double[5]{
                   1,   -3.770723788898,    5.519325819115,   -3.704298990066,
     0.9650811738991
};
        public struct Butterworth_4order
        {
            //the in signal
            public double x0;
            public double x1;
            public double x2;
            public double x3;
            public double x4;
            //the out signal
            public double y0;
            public double y1;
            public double y2;
            public double y3;
            public double y4;

        };

        //Butterworth_4order filter_butterworth;
        //Butterworth_4order
        public static void filter_init(ref Butterworth_4order channel)
        {
            channel.x0 = 0;
            channel.x1 = 0;
            channel.x2 = 0;
            channel.x3 = 0;
            channel.x4 = 0;
            channel.y0 = 0;
            channel.y1 = 0;
            channel.y2 = 0;
            channel.y3 = 0;
            channel.y4 = 0;
        }
        //typedef unsigned int uint16
        //Butterworth_4order * filter_struct,

        /**
         *
         */
        /**
         * @brief: the filter use to filter the signal off 50Hz interpret
         * @param [in] DIN: a pointer  to the start address of input buffer
         * @param [out] DOUT: a pointer to the start address of output buffer
         * @param [in] LEN: the Length of DIN and DOUT
         * @return: void
         * @see:
         * @note:
         */
        public static void filter(ref Butterworth_4order channel, List<double> DIN, ref List<double> DOUT, uint LEN)
        {
            double tmp;
            int i;
            for (i = 0; i < LEN; i++)
            {

                channel.x0 = DIN[i];
                tmp = NUM[0] * channel.x0 + NUM[1] * channel.x1 + NUM[2] * channel.x2 +
                        NUM[3] * channel.x3+ NUM[4] * channel.x4 - DEN[1] * channel.y1
                        - DEN[2] * channel.y2 - DEN[3] * channel.y3 - DEN[4] * channel.y4;
                tmp /= DEN[0];
                if (tmp >= 2.5)
                {
                    tmp = 2.5;
                }
                if (tmp <= -2.5)
                {
                    tmp = -2.5;
                }

                //*DOUT = (uint16)tmp;

                channel.x4 = channel.x3; channel.x3 = channel.x2; channel.x2 = channel.x1; channel.x1 = channel.x0;
                channel.y4 = channel.y3; channel.y3 = channel.y2; channel.y2 = channel.y1; channel.y1 = (double)tmp;

                DOUT.Add(tmp);


            }
        }

    }
}

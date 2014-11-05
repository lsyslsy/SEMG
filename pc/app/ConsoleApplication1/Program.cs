using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using SEMG.UIL;
using System.Collections.Specialized;
namespace ConsoleApplication1
{
    class Program
    {
        static void Main(string[] args)
        {
            Stopwatch stopWatch = new Stopwatch();
            FileHelper fh = new FileHelper(@"E:\sEMG\trunk\pc\app\ConsoleApplication1\bin\Debug\ss.txt");
            StringCollection sc = new StringCollection();
            for (int i = 0; i < 6000;i++ )
            {
                StringBuilder sb =new StringBuilder();
                for(int j =0;j<100;j++)
                {
                    sb.Append(DateTime.Now.ToShortDateString() + i.ToString()+"  ");
                }
                sc.Add(sb.ToString());
                    
            }
                stopWatch.Start();
                fh.Write(sc);
                fh.Close();
            // Get the elapsed time as a TimeSpan value.
            TimeSpan ts = stopWatch.Elapsed;

            // Format and display the TimeSpan value.
            string elapsedTime = String.Format("{0:00}:{1:00}:{2:00}.{3:00}",
                ts.Hours, ts.Minutes, ts.Seconds,
                ts.Milliseconds / 10);
            Console.WriteLine("RunTime " + elapsedTime);
            Console.Read();
        }
    }
}

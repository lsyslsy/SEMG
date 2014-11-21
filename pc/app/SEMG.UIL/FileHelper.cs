using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Data;
using System.Collections.Specialized;

namespace SEMG.UIL
{
    ///FileNotFoundException
    public class FileHelper
    {
        string filename = "";
        FileStream fs;
        StreamWriter sw;
        bool closed = false;

        public FileHelper(string fn)
        {
            filename = fn;
            if(File.Exists(filename))
            {
                throw new IOException("文件已存在");
            }
            
            fs = new FileStream(filename, FileMode.CreateNew, FileAccess.Write, FileShare.Read);
            sw = new StreamWriter(fs, Encoding.UTF8);          //write as UTF8 encoding

        }

        public void Flush()
        {
            lock (this)
            {
                sw.Flush();
            }
            
        }
        public void Write(string s)
        {
            lock (this)
            {
                if (closed)
                    return;
                sw.Write(s);
            }
        }
        public void WriteSC(StringCollection stringCollection)
        {
            lock (this)
            {
                if (closed)         //不锁了，防止死锁
                    return;
                foreach (string line in stringCollection)
                {
                    sw.WriteLine(line);
                }
            }
        }

        public void WriteLine(string s)
        {
            lock (this)
            {
                if (closed)
                    return;
                sw.WriteLine(s);
            }
        }
        public void Close()
        {
            lock (this)
            {
                closed = true;
                sw.Close();
                fs.Close();
            }
        }
    }

   public class ReadHelper
    {
        string filename = "";
        FileStream fs;
        StreamReader sr;
        public ReadHelper(string fn)
        {
            filename = fn;
            if(File.Exists(filename))
            {
                throw new IOException("文件已存在");
            }

            fs = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.None);
            sr = new StreamReader(fs, Encoding.UTF8);          //write as UTF8 encoding

        }

        public int ReadData(List<double>[] data)
        {
            //double[][] data = new double[10000][129];//data during 10s
            int row=0;
            string line;
            //skip 6 headlines
            for (int i = 0; i < 6; i++)
                sr.ReadLine();
            while ((line = sr.ReadLine()) != null)
            {
                string[] elements = line.Split('\t');
                for(int i = 1;i<Parameters.usingChannelCount+2;i++)
                {
                    data[i-1].Add(double.Parse(elements[i]));
                }
                if (row == 10000-1) break;
            }
          
            return row + 1;
        }
        //public void WriteSC(StringCollection stringCollection)
        //{
        //    foreach (string line in stringCollection)
        //    {
        //        sw.WriteLine(line);
        //    }
        //   // stringCollection.
        //}

        public string ReadLine()
        {
            return sr.ReadLine();
            
        }
        public void Close()
        {
            sr.Close();
            fs.Close();
        }
    }
}

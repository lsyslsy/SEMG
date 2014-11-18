package main

import (
	"bufio"
	"bytes"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"strings"
	"time"
)

//新的数据通道图
var datamap []int = []int{
	/* 0 */ 8, 9, 10, 11, 12, 13, 14, 15, //新的0-7,等同于老的8-15
	/* 0 */ 0, 1, 2, 3, 4, 5, 6, 7, //新的8-15，等同于老的0-7
	/* 1 */ 120, 121, 122, 123, 124, 125, 126, 127, //16-23
	/* 1 */ 112, 113, 114, 115, 116, 117, 118, 119, //24-31
	/* 2 */ 104, 105, 106, 107, 108, 109, 110, 111, //32-39
	/* 2 */ 96, 97, 98, 99, 100, 101, 102, 103, //40-47
	/* 3 */ 88, 89, 90, 91, 92, 93, 94, 95,
	/* 3 */ 80, 81, 82, 83, 84, 85, 86, 87,
	/* 4 */ 72, 73, 74, 75, 76, 77, 78, 79,
	/* 4 */ 64, 65, 66, 67, 68, 69, 70, 71,
	/* 5 */ 56, 57, 58, 59, 60, 61, 62, 63,
	/* 5 */ 48, 49, 50, 51, 52, 53, 54, 55,
	/* 6 */ 40, 41, 42, 43, 44, 45, 46, 47,
	/* 6 */ 32, 33, 34, 35, 36, 37, 38, 39,
	/* 7 */ 24, 25, 26, 27, 28, 29, 30, 31,
	/* 7 */ 16, 17, 18, 19, 20, 21, 22, 23,
}

var srcPath string
var dstPath string

func init() {
	flag.StringVar(&srcPath, "s", "", "source directory/file to convert from")
	flag.StringVar(&dstPath, "d", "", "destination directory/file to convert to")
}

func main() {
	//判断参数
	flag.Parse()
	if srcPath == "" || dstPath == "" {
		fmt.Printf("Usage: %s -s <source dir/file> -d <destination dir/file>\n", os.Args[0])
		os.Exit(1)
	}

	//确定源文件或目录存在
	info_s, err := os.Stat(srcPath)
	if os.IsNotExist(err) {
		fmt.Println(srcPath + " is not exists")
		os.Exit(1)
	}
	//any other errors?
	if err != nil {
		log.Fatal(err)
	}
	//目标文件或目录不存在
	info_d, err := os.Stat(srcPath)
	if os.IsExist(err) {
		fmt.Println(dstPath + " is  exists")
		os.Exit(1)
	}
	if info_s.IsDir() != info_d.IsDir() {
		fmt.Println("源和目标需要同时目录或文件")
		os.Exit(1)
	}

	//walk through
	filepath.Walk(srcPath, walkFn)
}

func walkFn(inFilename string, info os.FileInfo, err error) error {
	if err != nil {
		log.Fatal(err)
	}

	// skip directory
	if info.IsDir() {
		fmt.Println("enter directory:" + inFilename)
		return nil
	}

	rel := ""
	if rel, err = filepath.Rel(srcPath, inFilename); err != nil {
		return err
	}

	outFilename := filepath.Join(dstPath, rel)

	//目录不存在就创建一个
	if _, err = os.Stat(filepath.Dir(outFilename)); os.IsNotExist(err) {
		if err = os.MkdirAll(filepath.Dir(outFilename), os.ModeDir|os.ModePerm); err != nil {
			log.Fatal(err)
		}
	}

	//skip files without .txt extension
	if filepath.Ext(inFilename) != ".txt" {
		return nil
	}

	inFile, outFile := os.Stdin, os.Stdout

	//open in file
	if inFile, err = os.Open(inFilename); err != nil {
		log.Fatal(err)
	}
	defer inFile.Close()

	//create out file
	if outFile, err = os.Create(outFilename); err != nil {
		log.Fatal(err)
	}

	defer outFile.Close()
	if err = repair(inFile, outFile); err != nil {
		//fmt.Println(inFilename + err.Error())
		//return nil
		//return fmt.Errorf(inFilename + err.Error())
		fmt.Println(inFilename + err.Error())
		outFile.Close()
		return os.Remove(outFilename)
		//return nil
	}
	fmt.Println(inFilename + ":succeed")
	return nil
}

func repair(inFile io.Reader, outFile io.Writer) (err error) {
	reader := bufio.NewReader(inFile)
	writer := bufio.NewWriter(outFile)
	defer func() {
		if err == nil {
			err = writer.Flush()
		}
	}()

	lines := make([]string, 6)
	//Read first 6 lines
	//TODO \n windows有问题
	for i := 0; i < 6; i++ {
		lines[i], err = reader.ReadString('\n')
		if err == io.EOF { //less than 6 lines, skip  txt files not semg data
			return nil
		} else if err != nil {
			return err
		}
	}
	//fmt.Println("test1")
	//data format judge, skip txt files not semg data
	if !strings.Contains(lines[0], "Encoding") || !strings.Contains(lines[1], "SampleRate") {
		return nil
	}

	//判断日期 跳过9月1号以后数据
	t, err := time.Parse("2006/1/2", strings.Fields(lines[2])[1])
	if err != nil {
		t, err = time.Parse("2006-1-2", strings.Fields(lines[2])[1])
	}

	if err != nil {
		return fmt.Errorf(":file data format error")
	}
	if t.After(time.Date(2014, time.September, 1, 0, 0, 0, 0, time.UTC)) {
		return fmt.Errorf(":skip new version semg data")
	}
	//跳过已修复的
	if strings.Contains(lines[4], "repaired") {
		return fmt.Errorf(":skip 已修复数据")
	}
	lines[4] = strings.TrimRight(lines[4], "\r\n") + "(repaired)\r\n"

	//写headerlines
	for i := 0; i < 6; i++ {
		if _, err = writer.WriteString(lines[i]); err != nil {
			return err
		}
	}

	//读出来写进去
	eof := false
	for !eof {
		var line string
		line, err = reader.ReadString('\n')
		if err == io.EOF { //less than 6 lines
			eof = true
			err = nil
			return nil
		} else if err != nil {
			return err
		}

		fields := strings.Fields(line)
		if len(fields) != 130 {
			return fmt.Errorf(":数据错误")
		}

		var buffer bytes.Buffer
		buffer.WriteString(fields[0]) //write timestamp
		buffer.WriteString("\t")
		for i := 0; i < 128; i++ {
			buffer.WriteString(fields[datamap[i]+1])
			buffer.WriteString("\t")
		}
		buffer.WriteString(fields[129]) //write action
		buffer.WriteString("\r\n")
		if _, err = writer.WriteString(buffer.String()); err != nil {
			return err
		}
	}
	return nil
}

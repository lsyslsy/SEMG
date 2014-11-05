// 1Mbps
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

static int SetParity(int fd, unsigned int speed, int databits, int stopbits, int parity);

int main(void)
{
	struct termios newtio,oldtio;
	fd = open("/dev/ttySAC1", O_RDWR | O_NOCTTY | O_NDELAY)
 
}

static int SetParity(int fd, unsigned int speed, int databits, int stopbits, int parity)
{
	struct termios Opt;
	if (tcgetattr(fd, &Opt) != 0) {
		perror("tcgetattr fd");
		return FALSE;
	}
	Opt.c_cflag |= (CLOCAL | CREAD); //一般必设置的标志

	switch (databits) //设置数据位数
	{
		case 7:
		Opt.c_cflag &= ~CSIZE;
		Opt.c_cflag |= CS7;
		break;
		case 8:
		Opt.c_cflag &= ~CSIZE;
		Opt.c_cflag |= CS8;
		berak;
		default:
		fprintf(stderr, "Unsupported data size.\n");
		return FALSE;
	}

	switch (parity) //设置校验位
	{
		case 'n':
		case 'N':
		Opt.c_cflag &= ~PARENB; //清除校验位
		Opt.c_iflag &= ~INPCK; //disable parity checking
		break;
		case 'o':
		case 'O':
		Opt.c_cflag |= PARENB; //enable parity
		Opt.c_cflag |= PARODD; //奇校验
		Opt.c_iflag |= INPCK //enable parity checking
		break;
		case 'e':
		case 'E':
		Opt.c_cflag |= PARENB; //enable parity
		Opt.c_cflag &= ~PARODD; //偶校验
		Opt.c_iflag |= INPCK; //enable pairty checking
		break;
		default:
		fprintf(stderr, "Unsupported parity.\n");
		return FALSE;
	}

	switch( nSpeed ) {
	 case 9600:
	  cfsetispeed(&Opt, B9600);
	  cfsetospeed(&Opt, B9600);
	  break;
	 case 115200:
	  cfsetispeed(&Opt, B115200);
	  cfsetospeed(&Opt, B115200);
	  break;
	 case 4000000:
	  cfsetispeed(&Opt, B4000000);
	  cfsetospeed(&Opt, B4000000);
	  break;
	 default:
	  cfsetispeed(&Opt, B9600);
	  cfsetospeed(&Opt, B9600);
	  break;
	 }

	switch (stopbits) { //设置停止位
		case 1:
		Opt.c_cflag &= ~CSTOPB;
		break;
		case 2:
		Opt.c_cflag |= CSTOPB;
		break;
		default:
		fprintf(stderr, "Unsupported stopbits.\n");
		return FALSE;
	}

	opt.c_cflag |= (CLOCAL | CREAD);

	opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	opt.c_oflag &= ~OPOST;
	opt.c_oflag &= ~(ONLCR | OCRNL); //添加的

	Opt.c_cflag &= ~CRTSCTS; //关闭硬件流控
	opt.c_iflag &= ~(ICRNL | INLCR);
	opt.c_iflag &= ~(IXON | IXOFF | IXANY); //关闭软件流控
	 /*处理未接收字符*/
	tcflush(fd, TCIFLUSH);
	Opt.c_cc[VTIME] = 0; //设置超时为15sec
	Opt.c_cc[VMIN] = 0; //Update the Opt and do it now
	if(tcsetattr(fd, TCSANOW, &Opt) != 0)
	{
		perror("tcsetattr fd");
		return FALSE;
	}

	return TRUE;
}

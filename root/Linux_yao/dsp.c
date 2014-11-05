#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

static const char *device = "/dev/spidev0.0";
static uint8_t mode = 1;
static uint8_t bits = 8;
static uint32_t speed = 3022728;//4156250;//3200000;//4156250;//max clk:33250KHZ/8 256 division 4M
static uint16_t delay= 0;

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static uint32_t spi_write(int fd, uint8_t* buffer, uint32_t size)
{
    int status,i;
    uint8_t *ptr;
    ptr = buffer;
    while ((uint32_t)ptr - (uint32_t)buffer < size)
		{
			status = write(fd,ptr,1);
			ptr++;
                        //for (i=0; i<800; i++);/* delay for a while */
		}
    return (uint32_t)ptr - (uint32_t)buffer;
}

static void do_write(int fd)
{
	uint8_t *ptr;
        int8_t	buf[100] = {0};
	int	status,i,j;
        buf[0]=1;
        buf[1]=2;
        buf[2]=3;
        buf[3]=4;  
        buf[4]=5;
        buf[5]=6;	
	buf[6]=7;
        buf[7]=8;
	buf[8]=9;
	buf[9]=0;
        buf[98]=9;
        buf[99]=9;
      /* 
       while(1){
       ptr=buf;
       for (i=0;i<4;i++){
	   status = write(fd, ptr, 1);
           ptr++;
           for (j=0; j<40; j++);
          }
       usleep(10);
       }
*/
       
      while(1){
	   status = read(fd, buf, 5);
           
           usleep(10);           
//usleep(10);
     }

}

static uint32_t spi_read(int fd, uint8_t* buffer, uint32_t size)
{
    int status,i;
    uint8_t *ptr;
    ptr = buffer;
    while ((uint32_t)ptr - (uint32_t)buffer < size)
		{
			status = read(fd,ptr,1);
			ptr++;
                        for (i=0; i<800; i++);/* delay for a while */
		}
    return (uint32_t)ptr - (uint32_t)buffer;
}

static void do_read(int fd)
{
	int i;
	char tx=0xa1;
        uint8_t	   buf[2000]= {0};
	uint32_t   size = sizeof(buf);
      while(1)
      {
        write(fd, &tx, 1);
        //usleep();
        size = read(fd, buf, size);
	//printf("read done! size = %d, buf[0]=%d\n",size,buf[0]);
        printf("read:%d,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",size,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[1999]);
        //printf("read:%d,%x,%x,%x,%x,%x\n",size,buf[0],buf[1],buf[2],buf[3],buf[9]);
        usleep(20000);
       }
}


static void do_ack(int fd)
{
	int i;
	char tx=0xa1;
        uint8_t	   buf[2000]= {0};
	uint32_t   size = sizeof(buf);
      while(1)
      {
        write(fd, &tx, 1);
        for(i=0;i<10000;i++);
        //size = read(fd, buf, size);
        write(fd, &tx, 1);
        for(i=0;i<1000;i++);
        write(fd, &tx, 1);
	//printf("read done! size = %d, buf[0]=%x, buf[1]=%x\n",size,buf[0],buf[1]);
        //printf("read:%d,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",size,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[1998],buf[1999]);
        //printf("read:%d,%x,%x,%x,%x,%x\n",size,buf[0],buf[1],buf[2],buf[3],buf[9]);
  
       }
}

static void transfer(int fd)
{
	int ret;
	uint8_t tx[8] = {
		0x08, 0x02, 0x03, 0x04,
                0x05, 0x06, 0x07, 0x08
	};
	uint8_t rx[8] = {0};
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = 8,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
        while(1)
{
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");
	printf("read:%x,%x,%x,%x\n",rx[0],rx[1],rx[2],rx[7]);
        usleep(10);
}
}



static void do_msg(int fd)
{
	unsigned char tbuf[8];
        unsigned char rbuf[8];
        unsigned char *bp;
        struct spi_ioc_transfer	xfer[2];
	int status;
        tbuf[0]=1;
        tbuf[1]=2;
        tbuf[2]=3;
        tbuf[3]=4;
        tbuf[4]=1;
        //memset(xfer, 0, sizeof(xfer));
	xfer[0].tx_buf = (unsigned long) tbuf;
        xfer[0].rx_buf = (unsigned long) rbuf;
	xfer[0].len = 5;
       while(1){
	status = ioctl(fd, SPI_IOC_MESSAGE(1), xfer);
        printf("%d,write\n",status);
	if (status < 0) {
		printf("write wrong!\n");
		return;
	}
        usleep(10);
        }
	
}

static void set_dsp_spi(const char *device, int fd)
{
	int ret = 0;
   
        /*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");
        
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");
 
	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
}

int main(int argc, char **argv)
{
	int		fd;

	fd = open(device, O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}
       
	set_dsp_spi(device, fd);
    do_write(fd);
	//do_read(fd);
 	//do_ack(fd);
	close(fd);
	return 0;
}

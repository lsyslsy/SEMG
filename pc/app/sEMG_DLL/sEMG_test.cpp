#include <stdio.h>
#include "sEMG_DLL.h"

#include <unistd.h>

#define POINT_NUM 100
struct sEMGdata {
    double point[POINT_NUM];
};
sEMGdata mydata[10] = {{{0}}};
int usingChannelNum = -1;

void read_dev();
void open_dev();

int main(int argc, char const *argv[])
{
    open_dev();
    sleep(100);
    return 0;
}

/**
 * read data callback
 */
void read_dev()
{
    // 断开branch查询
    char stat[2];
    get_spi_stat(stat);
    // stat[0]！= 0为致命错误，应该报错退出
    // stat[1]中为共8个branch的连接状态，如果相应位不为0，则该branch断开，应该界面上显示出来。
    int count = 0;
    int ci = 0; // channel index
    for (ci = 0; ci < 128; ci++)
        count = get_sEMG_data(ci, 10, mydata); // count <= 10

        for (int i = 0; i < count; i++) {
            for (int k = 0; k < POINT_NUM; k++) {
                // do some thing
            }
    }
    printf("[TEST]: read frame counts: %d\n", count);
}

void open_dev()
{
    bool ret;
    const char *ip = "192.168.31.123";

    int sampleRate = -1;

    ret = sEMG_open(true, ip, 0, 100); // 1代表阻塞地等待semg设备打开
    if (ret) {
        usingChannelNum = get_channel_num(); // 获取通道数，应为128
        printf("[TEST]: channel num: %d\n", usingChannelNum);
        if (usingChannelNum != 128) {
            sEMG_close();
            return;
        }
        sampleRate = get_AD_rate(); // 单位为kps
        // 注意该回调函数将直接于采集线程中执行，所以不应处理复杂任务，
        // 推荐只读取数据，并通知其他线程进行数据处理
        set_data_notify(read_dev);
    } else {
        sEMG_close();
    }
}
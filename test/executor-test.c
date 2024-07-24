#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <bits/mman-linux.h>
#include <sys/time.h>
#include <time.h>

#define IOCTL_DEMO_MAGIC '\x66'
#define IOCTL_DEMO_VALGET_NUM _IOR(IOCTL_DEMO_MAGIC, 4, int)
#define IOCTL_DEMO_REGISTER_NUM _IOR(IOCTL_DEMO_MAGIC, 5, int)
#define IOCTL_DEMO_UNREGISTER_NUM _IOR(IOCTL_DEMO_MAGIC, 6, int)

/*
 * fallocate -l 1G ./bigFile-1G
 * sudo bash -c "sync && echo 3 > /proc/sys/vm/drop_caches"
 */

#define TIMER(val)                                  \
    do                                              \
    {                                               \
        struct timeval tm;                          \
        gettimeofday(&tm, NULL);                    \
        val = tm.tv_sec * 1000 + tm.tv_usec / 1000; \
    } while (0)

typedef struct __smart_madvise_ioctl_args
{
    unsigned long start;
    size_t len;
    void *user_semantics;
} smart_madvise_ioctl_args;

size_t SIZE = (size_t)1 * (size_t)1024 * (size_t)1024 * (size_t)1024;

int main(int argc, char *argv[])
{
    int need_smart_madvise = strtol(argv[1], NULL, 10);

    int fd = open("/dev/smart-madvise", O_RDWR); // 打开设备文件，以读写模式
    if (fd < 0)
    {
        perror("open");
        exit(-1);
    }
    int new_fd = open("./bigFile-1G", O_RDWR | O_EXCL);
    if (new_fd == -1)
    {
        perror("open error in readWithoutMadvise");
        exit(-1);
    }

    void *memory = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, new_fd, 0);
    close(new_fd);
    new_fd = -1;

    if (memory == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    printf("memory: %p\n", memory);
    int request_code = IOCTL_DEMO_REGISTER_NUM; // 根据设备文档定义的请求代码
    int data;                                   // 指向要发送或接收的数据的指针

    smart_madvise_ioctl_args obj = {
        .start = (unsigned long)memory,
        .len = SIZE,
        .user_semantics = NULL,
    };

    if (need_smart_madvise)
    {
        if (ioctl(fd, request_code, &obj) < 0)
        {
            perror("ioctl");
            exit(-1);
        }
    }

    printf("getchar:");
    getchar();
    printf("sleep 1\n");
    sleep(1);
    printf("sleep 2\n");
    sleep(1);

    // access memory
    long starttime, endtime;
    long nSum = 0;
    long *pp = (long *)memory;
    size_t it_num = SIZE / sizeof(long);
    TIMER(starttime);
    for (size_t i = 0; i < it_num; i++)
    {
        nSum += *pp;
        pp++;
    }
    TIMER(endtime);
    printf("read %s-madvise: %ld ms\n", (need_smart_madvise ? "w" : "w/o"), endtime - starttime);

    if (need_smart_madvise)
    {
        request_code = IOCTL_DEMO_UNREGISTER_NUM;
        if (ioctl(fd, request_code, &data) < 0)
        {
            perror("ioctl");
            exit(-1);
        }
    }

    printf("getchar to exit:");
    getchar();
    return 0;
}

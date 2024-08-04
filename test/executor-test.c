#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <bits/mman-linux.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>

#define IOCTL_DEMO_MAGIC '\x66'
#define IOCTL_DEMO_VALGET_NUM _IOR(IOCTL_DEMO_MAGIC, 4, int)
#define IOCTL_DEMO_REGISTER_NUM _IOR(IOCTL_DEMO_MAGIC, 5, int)
#define IOCTL_DEMO_UNREGISTER_NUM _IOR(IOCTL_DEMO_MAGIC, 6, int)
#define PAGE_SIZE 4096 // Define the typical page size for x86/x86_64

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

void drop_caches() {
    int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
    if (fd == -1) {
        perror("Failed to open /proc/sys/vm/drop_caches");
        return;
    }
    if (write(fd, "3", 1) != 1) {
        perror("Failed to drop caches");
    }
    close(fd);
}

// Function to shuffle an array
void shuffle(size_t *array, size_t n)
{
    if (n > 1)
    {
        for (size_t i = n - 1; i > 0; i--)
        {
            size_t j = rand() % (i + 1);
            size_t t = array[i];
            array[i] = array[j];
            array[j] = t;
        }
    }
}

typedef struct __smart_madvise_ioctl_args
{
    unsigned long start;
    size_t len;
    void *user_semantics;
} smart_madvise_ioctl_args;

size_t SIZE = (size_t)1 * (size_t)1024 * (size_t)1024 * (size_t)1024;

int main(int argc, char *argv[])
{
    int smart_madvise_type = strtol(argv[1], NULL, 10);

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

    if (smart_madvise_type == -1)
    {
        if (ioctl(fd, request_code, &obj) < 0)
        {
            perror("ioctl");
            exit(-1);
        }
    }
    else if (smart_madvise_type == 1){
        madvise(memory, SIZE, MADV_RANDOM);
    }
    else if (smart_madvise_type == 2){
        madvise(memory, SIZE, MADV_SEQUENTIAL);
    }
    else if (smart_madvise_type == 0){
        madvise(memory, SIZE, MADV_NORMAL);
    }
    else{
        printf("Invalid smart madvise type\n");
        exit(-1);
    }
    printf("Sequential read\n");
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
      // printf("Dropping caches...\n");
      TIMER(starttime);
    for (size_t i = 0; i < it_num; i++)
    {
        nSum += *pp;
        pp++;
    }
    TIMER(endtime);
    printf("Dropping caches...\n");
    drop_caches();

    printf("Random read\n");
      printf("getchar:");
    getchar();
     printf("sleep 1\n");
    sleep(1);
    // access memory randomly
    size_t num_pages = SIZE / PAGE_SIZE;
    size_t *page_indices = malloc(num_pages * sizeof(size_t));
    if (!page_indices)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Initialize the page indices
    for (size_t i = 0; i < num_pages; i++)
    {
        page_indices[i] = i;
    }

    // Shuffle the page indices to simulate random access
    shuffle(page_indices, num_pages);

    TIMER(starttime);
     // size_t it_num = SIZE / sizeof(long);
   
    for (size_t i = 0; i < num_pages; i++) {
        size_t page_idx = page_indices[i];
        long *data = (long *)(memory + page_idx * PAGE_SIZE);
        nSum += *data;  // Simulate a read from the page
    }

    TIMER(endtime);
    printf("Random read %s-madvise: %ld ms with type ", (smart_madvise_type ? "w" : "w/o"), endtime - starttime);
    if (smart_madvise_type == -1)
        printf("MADV_SMART\n");
    else if (smart_madvise_type == 0){
        printf("MADV_NORMAL\n");
    }
    else if (smart_madvise_type == 1){
        printf("MADV_RANDOM\n");
    }
    else if (smart_madvise_type == 2){
        printf("MADV_SEQUENTIAL\n");
    }
    

    free(page_indices);
    if (smart_madvise_type == 0)
    {
        request_code = IOCTL_DEMO_UNREGISTER_NUM;
        if (ioctl(fd, request_code, &data) < 0)
        {
            perror("ioctl");
            exit(-1);
        }
    }
    sleep(1);

}
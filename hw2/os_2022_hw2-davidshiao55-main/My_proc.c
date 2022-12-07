#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/string.h>

#define MAX_THREAD 32
#define PROC_ENTRY_FILENAME "buffer2k"
#define PROCFS_MAX_SIZE 4096
#define PROCFS_LOCAL_SIZE 512
#define PID 0
#define TID 1
#define TIME 2
#define NTHREAD 3

typedef struct message
{
    int type;
    union
    {
        int pid;
        int nt;
        long int tid;
    };
    int time;
} message;

/**
 * The buffer (4k) for this module
 */
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;
static int pid = 0;
static int numThread;
static int threadcount = 0;
static int finisht = 0;
static long unsigned int tid[MAX_THREAD];
static int time_taken[MAX_THREAD];
static int cs[MAX_THREAD];

MODULE_LICENSE("GPL");

static struct proc_dir_entry *proc_file;

/**
 * @brief Read data out of the buffer
 */
static ssize_t my_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
    if (*offset || procfs_buffer_size == 0)
    {
        printk(KERN_INFO "procfs_read: END\n");
        *offset = 0;
        return 0;
    }
    procfs_buffer_size = min(procfs_buffer_size, length);
    if (copy_to_user(buffer, procfs_buffer, procfs_buffer_size))
        return -EFAULT;
    *offset += procfs_buffer_size;

    printk(KERN_INFO "procfs_read: read %lu bytes\n", procfs_buffer_size);
    return procfs_buffer_size;
}

/**
 * @brief Write data from the buffer
 */
static ssize_t my_write(struct file *file, const char *buffer, size_t len, loff_t *off)
{
    char local_buffer[PROCFS_LOCAL_SIZE];
    struct task_struct *the_process;
    struct task_struct *the_thread;
    message mess;
    int i;

    if (procfs_buffer_size > PROCFS_MAX_SIZE)
    {
        procfs_buffer_size = PROCFS_MAX_SIZE;
    }

    if (copy_from_user(&mess, buffer, sizeof(message)))
    {
        return -EFAULT;
    }

    // cast message
    switch (mess.type)
    {
    case NTHREAD:
        numThread = mess.nt;
        printk(KERN_INFO "procfs_write: numthread is %d\n", numThread);
        // reset
        pid = 0;
        finisht = 0;
        threadcount = 0;
        procfs_buffer_size = 0;
        break;
    case PID:
        pid = mess.pid;
        printk(KERN_INFO "procfs_write: pid is %d\n", pid);
        break;
    case TID:
        printk(KERN_INFO "procfs_write: tid is %ld\n", mess.tid);
        tid[threadcount] = mess.tid;

        // find recieved tid context switch time
        rcu_read_lock();
        for_each_process(the_process)
        {
            if (task_pid_nr(the_process) == pid)
            {
                for_each_thread(the_process, the_thread)
                {
                    if (the_thread->pid == tid[threadcount])
                    {
                        cs[threadcount] = the_thread->nvcsw + the_thread->nivcsw;
                        break;
                    }
                }
                break;
            }
        }
        rcu_read_unlock();

        threadcount++;
        break;
    case TIME:
        for (i = 0; i < threadcount; i++)
        {
            if (mess.tid == tid[i])
            {
                time_taken[i] = mess.time;
                printk(KERN_INFO "procfs_write: tid %lu finish in %d(ms)\n", tid[i], time_taken[i]);
                break;
            }
        }
        finisht++;
        break;
    }

    // all threads have been recieved, write to thread_info
    if (finisht == numThread)
    {
        printk(KERN_INFO "procfs_write: recieve all threads\n");
        sprintf(local_buffer, "PID:%d\n", pid);
        strncpy(procfs_buffer + procfs_buffer_size, local_buffer, strlen(local_buffer));
        procfs_buffer_size += strlen(local_buffer);
        for (i = 0; i < numThread; i++)
        {
            sprintf(local_buffer, "ThreadID:%lu Time:%d(ms) context switch time:%d\n", tid[i], time_taken[i], cs[i]);
            strncpy(procfs_buffer + procfs_buffer_size, local_buffer, strlen(local_buffer));
            procfs_buffer_size += strlen(local_buffer);
        }
        procfs_buffer[procfs_buffer_size++] = '\0';
    }

    return procfs_buffer_size;
}

static struct proc_ops fops = {
    .proc_read = my_read,
    .proc_write = my_write,
};

static int __init my_init(void)
{
    proc_file = proc_create("thread_info", 0666, NULL, &fops);
    if (proc_file == NULL)
    {
        printk(KERN_INFO "My_proc: error creating /proc/thread_info\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "My_proc: created /proc/thread_info\n");
    return 0;
}

static void __exit my_exit(void)
{
    printk(KERN_INFO "My_proc: Removing /proc/thread_info\n");
    proc_remove(proc_file);
}

module_init(my_init);
module_exit(my_exit);
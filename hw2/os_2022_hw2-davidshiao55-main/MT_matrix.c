#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/syscall.h>

#define BUF_SIZE 4096
#define MAXLENGTH 50
#define PID 0
#define TID 1
#define TIME 2
#define NTHREAD 3

// use to pass communicate between kernel & user
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

// use to pass information to thread
typedef struct threadParameters
{
    int workload;
    int r1;
    int c1;
    int r2;
    int c2;
    int file;
    int **rows;
    int **cols;
} threadParameters;

// use to catch return result from thread
typedef struct threadresult
{
    long int tid;
    int *result;
} threadresult;

pthread_mutex_t lock;

void *mult(void *arg);

int main(int argc, char *argv[])
{
    FILE *file1, *file2, *file3;
    int file4;
    pid_t pid = getpid();
    message mess;

    // get numThread from argv
    int numThread = 0;
    for (int i = 0; argv[1] && argv[1][i]; i++)
    {
        numThread *= 10;
        numThread += argv[1][i] - '0';
    }

    int r1, c1, r2, c2;
    file1 = fopen(argv[2], "r");
    file2 = fopen(argv[3], "r");
    file3 = fopen("result.txt", "w");
    file4 = open("/proc/thread_info", O_RDWR);

    if (!file1 || !file2 || !file3 || file4 == -1)
    {
        fprintf(stderr, "file can't be open\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return 1;
    }

    // send numThread & pid to kernel
    mess.type = NTHREAD;
    mess.nt = numThread;
    write(file4, &mess, sizeof(message));
    mess.type = PID;
    mess.pid = pid;
    write(file4, &mess, sizeof(message));

    // read matrix from file
    fscanf(file1, "%d %d", &r1, &c1);
    fscanf(file2, "%d %d", &r2, &c2);
    int **matA = (int **)malloc(sizeof(int *) * r1);
    int **matB = (int **)malloc(sizeof(int *) * r2);
    int *ma = (int *)malloc(sizeof(int) * r1 * c1);
    int *mb = (int *)malloc(sizeof(int) * r2 * c2);
    for (int i = 0; i < r1; i++)
    {
        matA[i] = &ma[i * c1];
        for (int j = 0; j < c1; j++)
        {
            fscanf(file1, "%d", &matA[i][j]);
        }
    }

    for (int i = 0; i < r2; i++)
    {
        matB[i] = &mb[i * c2];
        for (int j = 0; j < c2; j++)
        {
            fscanf(file2, "%d", &matB[i][j]);
        }
    }
    fclose(file1);
    fclose(file2);

    int totalElements = r1 * c2;
    struct timeval begin[numThread], end[numThread];
    int workload[numThread];
    pthread_t *threads = (pthread_t *)malloc(numThread * sizeof(pthread_t));

    // compute workload of each thread
    for (int i = 0; i < numThread; i++)
    {
        workload[i] = totalElements / numThread;
    }
    workload[numThread - 1] += totalElements % numThread;

    // pack information that the thread need to compute matrix multiplication
    threadParameters *tp[numThread];
    int count = 0;
    for (int i = 0; i < numThread; i++)
    {
        tp[i] = (threadParameters *)malloc(sizeof(threadParameters));
        tp[i]->rows = (int **)malloc(sizeof(int *) * workload[i]);
        tp[i]->cols = (int **)malloc(sizeof(int *) * workload[i]);
        tp[i]->workload = workload[i];
        tp[i]->r1 = r1;
        tp[i]->c1 = c1;
        tp[i]->r2 = r2;
        tp[i]->c2 = c2;
        tp[i]->file = file4;

        for (int j = 0; j < workload[i]; j++)
        {
            tp[i]->rows[j] = &matA[count / c2][0];
            tp[i]->cols[j] = &matB[0][count % c2];
            count++;
        }

        // start clock
        gettimeofday(&begin[i], 0);
        // create thread
        pthread_create(&threads[i], NULL, mult, (void *)(tp[i]));
    }

    fprintf(file3, "%d %d\n", r1, c2);
    count = 1;
    for (int i = 0; i < numThread; i++)
    {
        // void pointer to catch return result
        void *r;
        // Joining all threads and collecting return value
        pthread_join(threads[i], &r);
        threadresult *tr = (threadresult *)r;

        // record time
        gettimeofday(&end[i], 0);
        long seconds = end[i].tv_sec - begin[i].tv_sec;
        long microseconds = end[i].tv_usec - begin[i].tv_usec;
        double elapsed = seconds + microseconds * 1e-6;
        // send finish time to kernel
        mess.type = TIME;
        mess.tid = tr->tid;
        mess.time = (int)(elapsed * 1000);

        pthread_mutex_lock(&lock);
        write(file4, &mess, sizeof(message));
        pthread_mutex_unlock(&lock);

        // output multiplication result to "result.txt"
        int *p = tr->result;
        for (int j = 0; j < workload[i]; j++)
        {
            fprintf(file3, "%d ", p[j]);
            if (count % c2 == 0)
                fprintf(file3, "\n");
            count++;
        }

        free(tr->result);
        free(tp[i]->rows);
        free(tp[i]->cols);
        free(tp[i]);
        free(r);
    }

    // read from /proc/thread_info
    char threadInfo[BUF_SIZE];
    read(file4, threadInfo, BUF_SIZE);
    printf("%s", threadInfo);

    pthread_mutex_destroy(&lock);
    free(matA);
    free(matB);
    free(ma);
    free(mb);
    fclose(file3);
    close(file4);
    return 0;
}

// handle workload[i] elements matrix multiplication
void *mult(void *arg)
{
    message mess;
    threadParameters *tp = (threadParameters *)arg;
    threadresult *tr = (threadresult *)malloc(sizeof(threadresult));

    // cast parameter
    int workload = tp->workload;
    int r1 = tp->r1;
    int c1 = tp->c1;
    int r2 = tp->r2;
    int c2 = tp->c2;
    int file = tp->file;
    int **rows = tp->rows;
    int **cols = tp->cols;

    tr->tid = syscall(__NR_gettid);
    tr->result = (int *)malloc(sizeof(int) * workload);

    // compute matrix multiplication
    for (int i = 0; i < workload; i++)
    {
        int sum = 0;
        for (int j = 0; j < c1; j++)
        {
            sum += rows[i][j] * cols[i][j * c2];
        }
        (tr->result)[i] = sum;
    }
    // send threadID to kernel
    pthread_mutex_lock(&lock);
    mess.type = TID;
    mess.tid = tr->tid;
    write(file, &mess, sizeof(message));
    pthread_mutex_unlock(&lock);

    pthread_exit(tr);
}

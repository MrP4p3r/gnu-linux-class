#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <semaphore.h>

#define FTOK_FILE "./lab6-forking"

#define SHMB_MSG_LEN

typedef struct shm_block_struct {
    sem_t sem; // Общий семафор
    sem_t sem_new_msg; // Семафор для получения нового сообщения
    sem_t sem_send_msg; // Семафор для доступа к отправке сообщения
    int sender_proc_num;
    int sender_proc_pid;
    char msg[SHMB_MSG_LEN];
} shm_block_struct;

int main(int argc, char *argv[]);

void proc_zero(shm_block_struct *shm_block);
void proc_fork(shm_block_struct *shm_block, int proc_num, int proc_pid);

int main(int argc, char *argv[])
{
    // Инициализация блока разделяемой памяти
    key_t key;
    int shm_id;
    shm_block_struct *shm_block;

    key = ftok(FTOK_FILE, 1);
    printf("%i\n\n", key);
    printf("%s\n\n", FTOK_FILE);
    shm_id = shmget(key, sizeof(shm_block_struct), 0666 | IPC_CREAT);
    shm_block = (shm_block_struct *) shmat(shm_id, 0, 0);

    // Общий семафор
    sem_init(&shm_block->sem, /* pshared = */ 1, /* value = */ 1);
    // Семафор для получения нового сообщения
    sem_init(&shm_block->sem_new_msg, /* pshared = */ 1, /* value = */ 0);
    // Семафор для доступа к отправке сообщения
    sem_init(&shm_block->sem_send_msg, /* pshared = */ 1, /* value = */ 1);

    int frk, proc_pid, proc_num;
    for (int i = 0; i < 3; i++) {
        frk = fork();
        if (frk == 0) {
            proc_pid = getpid();
            proc_num = i+1;
            break;
        }
    }

    if (frk == -1) {
        printf("LOLSHTO?\n");
        return 1;
    }
    if (frk == 0)
        proc_fork(shm_block, proc_num, proc_pid);
    else
        proc_zero(shm_block);

    sem_destroy(&shm_block->sem);
    sem_destroy(&shm_block->sem_new_msg);
    sem_destroy(&shm_block->sem_send_msg);
    shmctl(shm_id, IPC_RMID, 0);

    return 0;
}

void proc_zero(shm_block_struct *m)
{
    sem_wait(&m->sem);
    printf("Main Proc: Waiting for 3 messages from 3 forks\n\n");
    sem_post(&m->sem);

    short msg_rec[3] = {0, 0, 0};
    while (!msg_rec[0] || !msg_rec[1] || !msg_rec[2]){
        sem_wait(&m->sem_new_msg); // Ожидание новой сообщеньки
        sem_wait(&m->sem);
        printf("Main Proc: Received message from #%i with PID %i:\n%s\n\n",
            m->sender_proc_num, m->sender_proc_pid, m->msg);
        msg_rec[m->sender_proc_num-1] = 1;
        sem_post(&m->sem);
        sem_post(&m->sem_send_msg); // Разрешение на отправку сообщенек
    }

    sem_wait(&m->sem);
    printf("Main Proc: Received all messages! Return.\n\n");
    sem_post(&m->sem);

    return;
}

void proc_fork(shm_block_struct *m, int proc_num, int proc_pid)
{
    sem_wait(&m->sem);
    printf("Process #%i: Hey, I'm PID %i, look at me!\n\n", proc_num, proc_pid);
    sem_post(&m->sem);

    // Отправка сообщения
    // Жду разрешения на отправку сообщения
    sem_wait(&m->sem_send_msg);
    m->sender_proc_num = proc_num;
    m->sender_proc_pid = proc_pid;
    sprintf((char*) &m->msg, "This is a message from the process #%i with PID %i",
        m->sender_proc_num, m->sender_proc_pid);
    // Даю разрешение на получение
    sem_post(&m->sem_new_msg);

    sem_wait(&m->sem);
    printf("Process #%i: Sent a message.\n\n", proc_num);
    sem_post(&m->sem);

    return;
}

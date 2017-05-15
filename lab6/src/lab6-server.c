#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#include "shared.h"

int main(int argc, char *argv[]);

void proc_server(shm_block_struct *m);

int main(int argc, char *argv[])
{
    // Инициализация блока разделяемой памяти
    key_t key;
    int shm_id;
    shm_block_struct *shm_block;

    key = new_key();
    if (key == -1) {
        printf("ftok() failed for some reason\n\n");
        return 1;
    }
    shm_id = shmget(key, sizeof(shm_block_struct), 0666 | IPC_CREAT);
    shm_block = (shm_block_struct *) shmat(shm_id, 0, 0);

    // Общий семафор
    sem_init(&shm_block->sem, /* pshared = */ 1, /* value = */ 1);
    // Семафор для получения нового сообщения
    sem_init(&shm_block->sem_new_msg, /* pshared = */ 1, /* value = */ 0);
    // Семафор для доступа к отправке сообщения
    sem_init(&shm_block->sem_send_msg, /* pshared = */ 1, /* value = */ 1);

    proc_server(shm_block);

    sem_destroy(&shm_block->sem);
    sem_destroy(&shm_block->sem_new_msg);
    sem_destroy(&shm_block->sem_send_msg);
    shmctl(shm_id, IPC_RMID, 0);

    return 0;
}

void proc_server(shm_block_struct *m)
{
    sem_wait(&m->sem);
    printf("Main Proc: Waiting for 5 messages\n\n");
    sem_post(&m->sem);

    for (int i = 0; i < 5; i++) {
        sem_wait(&m->sem_new_msg); // Ожидание новой сообщеньки
        sem_wait(&m->sem);
        printf("Main Proc: Received message from client process with PID %i:\n%s\n\n",
            m->sender_proc_pid, m->msg);
        sem_post(&m->sem);
        sem_post(&m->sem_send_msg); // Разрешение на отправку сообщенек
    }

    sem_wait(&m->sem);
    printf("Main Proc: Received five messages! That's enough. Return.\n\n");
    sem_post(&m->sem);

    return;
}

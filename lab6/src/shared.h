#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#define FTOK_FILE "/tmp/lab6-token"
#define PROJ_ID 6

#define SHMB_MSG_LEN 256

typedef struct shm_block_struct {
    sem_t sem; // Общий семафор
    sem_t sem_new_msg; // Семафор для получения нового сообщения
    sem_t sem_send_msg; // Семафор для доступа к отправке сообщения
    int sender_proc_pid;
    char msg[SHMB_MSG_LEN];
} shm_block_struct;

key_t new_key()
{
    FILE *fp;
    if (!(fp = fopen(FTOK_FILE, "w"))) return -1;

    key_t key = ftok(FTOK_FILE, PROJ_ID);

    if (key == -1) return key;

    fprintf(fp, "%i", key);
    fclose(fp);

    return key;
}

key_t load_key()
{
    key_t key;

    FILE *fp;
    if (!(fp = fopen(FTOK_FILE, "r"))) return -1;
    fscanf(fp, "%i", &key);
    fclose(fp);

    return key;
}

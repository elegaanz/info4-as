#include <nanvix/pm.h>
#include <nanvix/klib.h>

typedef struct {
    int counter;
    unsigned int key;
    struct process* waiting_processes;
} semaphore;

#define MAX_SEM 256
PRIVATE semaphore SEMAPHORES[MAX_SEM];
PRIVATE int n_sem = 0;

PUBLIC int sys_semget(unsigned key) {
    for (int i = 0; i < n_sem; i++) {
        if (SEMAPHORES[i].key == key) {
            struct process *p = SEMAPHORES[i].waiting_processes;
            while (p) {
                p = p->next;
            }
            if (p != curr_proc) {
                p->next = curr_proc;
            }
            return i;
        }
    }

    if (n_sem >= MAX_SEM) {
        return -1;
    }

    SEMAPHORES[n_sem].counter = 0;
    SEMAPHORES[n_sem].key = key;
    SEMAPHORES[n_sem].waiting_processes = NULL;

    int id = n_sem;
    n_sem++;

    return id;
}

#define GETVAL   0
#define SETVAL   1
#define IPC_RMID 3

PUBLIC int sys_semctl(int semid, int cmd, int val) {
    switch (cmd) {
        case GETVAL:
            return SEMAPHORES[semid].counter;
        case SETVAL:
            SEMAPHORES[semid].counter = val;
            break;
        case IPC_RMID:
            if (SEMAPHORES[semid].waiting_processes == NULL) {
                for (int i = n_sem - 1; i > semid; i--) {
                    kmemcpy(&SEMAPHORES[i - 1], &SEMAPHORES[i], sizeof(semaphore));
                }
                n_sem--;
            }
            break;
        default:
            return -1;
    }
    return 0;
}

PUBLIC int sys_semop(int semid, int op) {
    if (op == 0) return -1;

    disable_interrupts();

    int up = op > 0;
    if (up) {
        SEMAPHORES[semid].counter++;
        if (SEMAPHORES[semid].counter == 0) {
            wakeup(&SEMAPHORES[semid].waiting_processes);
        }
    } else {
        SEMAPHORES[semid].counter--;

        if (SEMAPHORES[semid].counter < 0) {
            sleep(&SEMAPHORES[semid].waiting_processes, curr_proc->priority);
        }
    }

    enable_interrupts();

    return 0;
}
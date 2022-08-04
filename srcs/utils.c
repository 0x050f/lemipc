#include <lemipc.h>

int		sem_lock(int sem_id)
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;
	sem_b.sem_flg = 0;
	if (semop(sem_id, &sem_b, 1) < 0)
	{
		dprintf(STDERR_FILENO, "%s: semop(): %s\n", PRG_NAME, strerror(errno));
		return(EXIT_FAILURE);
	}
	return(EXIT_SUCCESS);
}

int		sem_unlock(int sem_id)
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = 1;
	sem_b.sem_flg = 0;
	if (semop(sem_id, &sem_b, 1) < 0)
	{
		dprintf(STDERR_FILENO, "%s: semop(): %s\n", PRG_NAME, strerror(errno));
		return(EXIT_FAILURE);
	}
	return(EXIT_SUCCESS);
}

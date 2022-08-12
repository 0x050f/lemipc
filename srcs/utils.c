#include "lemipc.h"

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

int		sem_trylock(int sem_id)
{
	struct timespec timeout;
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = -1;
	sem_b.sem_flg = 0;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 1000;

	return (semtimedop(sem_id, &sem_b, 1, &timeout));
}

int		sem_tryunlock(int sem_id)
{
	if (semctl(sem_id, 0, GETVAL, 0) == 0)
		return (sem_unlock(sem_id));
	return (EXIT_FAILURE);
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

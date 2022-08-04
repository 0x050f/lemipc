#include "lemipc.h"

struct ipc	g_ipc;

void		signal_handler(int signum)
{
	(void)signum;
	if (g_ipc.game)
		exit_game(&g_ipc);
	printf("\b\bLeaving the game...\n");
	exit(EXIT_SUCCESS);
}

int			create_ipc(struct ipc *ipc, key_t key)
{
	ipc->shm_id = shmget(key, sizeof(struct game), IPC_CREAT | SHM_R | SHM_W);
	if (ipc->shm_id < 0)
	{
		dprintf(STDERR_FILENO, "%s: shmget(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	ipc->sem_id = semget(key, 1, IPC_CREAT | SHM_R | SHM_W);
	if (ipc->sem_id < 0)
	{
		dprintf(STDERR_FILENO, "%s: semget(): %s\n", PRG_NAME, strerror(errno));
		shmctl(ipc->shm_id, IPC_RMID, 0);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int			lemipc(struct ipc *ipc)
{
	key_t			key;

	if (signal(SIGINT, signal_handler) == SIG_ERR)
	{
		dprintf(STDERR_FILENO, "%s: signal(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if ((key = ftok("/tmp", 42)) < 0)
	{
		dprintf(STDERR_FILENO, "%s: ftok(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if ((ipc->sem_id = semget(key, 0, 0)) < 0)
	{
		if (create_ipc(ipc, key) == EXIT_FAILURE)
			return (EXIT_FAILURE);
		create_game(ipc);
		sem_unlock(ipc->sem_id);
	}
	else
	{
		ipc->shm_id = shmget(key, 0, 0);
		ipc->game = shmat(ipc->shm_id, NULL, 0);
	}
	join_game(ipc);
	exit_game(ipc);
	return (EXIT_SUCCESS);
}

bool		check_args(int argc, char *argv[])
{
	if (argc != 2)
	{
		dprintf(STDERR_FILENO, "usage: ./%s team_number\n", PRG_NAME);
		return (false);
	}
	int len = strlen(argv[1]);
	if (len != 1 || argv[1][0] < '0' || argv[1][0] > '9')
	{
		dprintf(STDERR_FILENO, "%s: Wrong argument - team_number must be [0-9]\n", PRG_NAME);
		return (false);
	}
	return (true);
}

int			main(int argc, char *argv[])
{
	if (!check_args(argc, argv))
		return (EXIT_FAILURE);
	g_ipc.player.team = atoi(argv[1]);
	g_ipc.player.pos_x = 0;
	g_ipc.player.pos_y = 0;
	g_ipc.shm_id = -1;
	g_ipc.sem_id = -1;
	g_ipc.game = NULL;
	return (lemipc(&g_ipc));
}

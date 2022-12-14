#include "lemipc.h"

struct ipc	g_ipc;

void		signal_end_game(int signum)
{
	(void)signum;
	struct player	*players;

	if (g_ipc.game)
	{
		sem_tryunlock(g_ipc.sem_id[MAP]);
		sem_tryunlock(g_ipc.sem_id[PLAY]);
		players = g_ipc.game->players;
		sem_lock(g_ipc.sem_id[PLAYERS]);
		for (size_t i = 0; i < MAX_PLAYERS; i++)
		{
			if (players[i].pid != -1 && players[i].pid != g_ipc.player.pid)
				kill(players[i].pid, SIGINT);
			players[i].pid = -1;
		}
		sem_unlock(g_ipc.sem_id[PLAYERS]);
		printf("\b\bExit the game...\n");
		exit_game(&g_ipc);
	}
	exit(EXIT_SUCCESS);
}

int			create_ipc(struct ipc *ipc, key_t keys[3])
{
	ipc->shm_id = shmget(keys[0], sizeof(struct game), IPC_CREAT | SHM_R | SHM_W);
	if (ipc->shm_id < 0)
	{
		dprintf(STDERR_FILENO, "%s: shmget(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	ipc->sem_id[MAP] = semget(keys[0], 1, IPC_CREAT | SHM_R | SHM_W);
	if (ipc->sem_id[MAP] < 0)
	{
		dprintf(STDERR_FILENO, "%s: semget(): %s\n", PRG_NAME, strerror(errno));
		shmctl(ipc->shm_id, IPC_RMID, 0);
		return (EXIT_FAILURE);
	}
	ipc->sem_id[PLAYERS] = semget(keys[1], 1, IPC_CREAT | SHM_R | SHM_W);
	if (ipc->sem_id[PLAYERS] < 0)
	{
		dprintf(STDERR_FILENO, "%s: semget(): %s\n", PRG_NAME, strerror(errno));
		semctl(ipc->sem_id[MAP], IPC_RMID, 0);
		shmctl(ipc->shm_id, IPC_RMID, 0);
		return (EXIT_FAILURE);
	}
	ipc->sem_id[PLAY] = semget(keys[2], 1, IPC_CREAT | SHM_R | SHM_W);
	if (ipc->sem_id[PLAYERS] < 0)
	{
		dprintf(STDERR_FILENO, "%s: semget(): %s\n", PRG_NAME, strerror(errno));
		semctl(ipc->sem_id[MAP], IPC_RMID, 0);
		semctl(ipc->sem_id[PLAYERS], IPC_RMID, 0);
		shmctl(ipc->shm_id, IPC_RMID, 0);
		return (EXIT_FAILURE);
	}
	ipc->mq_id = msgget(keys[0], IPC_CREAT | SHM_R | SHM_W);
	if (ipc->mq_id < 0)
	{
		dprintf(STDERR_FILENO, "%s: msgget(): %s\n", PRG_NAME, strerror(errno));
		semctl(ipc->sem_id[MAP], IPC_RMID, 0);
		semctl(ipc->sem_id[PLAYERS], IPC_RMID, 0);
		semctl(ipc->sem_id[PLAY], IPC_RMID, 0);
		shmctl(ipc->shm_id, IPC_RMID, 0);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int			setup_signals(void)
{
	if (signal(SIGINT, signal_end_game) == SIG_ERR)
	{
		dprintf(STDERR_FILENO, "%s: signal(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if (signal(SIGTERM, signal_end_game) == SIG_ERR)
	{
		dprintf(STDERR_FILENO, "%s: signal(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if (signal(SIGQUIT, signal_end_game) == SIG_ERR)
	{
		dprintf(STDERR_FILENO, "%s: signal(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int			lemipc(struct ipc *ipc)
{
	int				nb_teams;
	key_t			keys[3];

	if (setup_signals() == EXIT_FAILURE)
		return (EXIT_FAILURE);
	for (size_t i = 0; i < 3; i++)
	{
		if ((keys[i] = ftok("/tmp", i + 42)) < 0)
		{
			dprintf(STDERR_FILENO, "%s: ftok(): %s\n", PRG_NAME, strerror(errno));
			return (EXIT_FAILURE);
		}
	}
	if ((ipc->sem_id[MAP] = semget(keys[0], 0, 0)) < 0)
	{
		if (create_ipc(ipc, keys) == EXIT_FAILURE)
			return (EXIT_FAILURE);
		create_game(ipc);
		sem_unlock(ipc->sem_id[MAP]);
		sem_unlock(ipc->sem_id[PLAYERS]);
		sem_unlock(ipc->sem_id[PLAY]);
	}
	else
	{
		ipc->shm_id = shmget(keys[0], 0, 0);
		ipc->mq_id = msgget(keys[0], 0);
		ipc->game = shmat(ipc->shm_id, NULL, 0);
		ipc->sem_id[PLAYERS] = semget(keys[1], 0, 0);
		ipc->sem_id[PLAY] = semget(keys[2], 0, 0);
	}
	if (setup_chatbox(ipc) == EXIT_FAILURE)
		return (EXIT_FAILURE);
	nb_teams = count_nb_teams(ipc);
	if (join_game(ipc) == EXIT_SUCCESS)
	{
		if (nb_teams < 2)
			waiting_game(ipc);
		play_game(ipc);
		exit_game(ipc);
	}
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
	g_ipc.sem_id[MAP] = -1;
	g_ipc.sem_id[PLAYERS] = -1;
	g_ipc.game = NULL;
	g_ipc.chatbox = NULL;
	return (lemipc(&g_ipc));
}

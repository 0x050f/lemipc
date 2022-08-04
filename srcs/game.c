#include <lemipc.h>

int		play_game(struct ipc *ipc)
{
	(void)ipc;
	sleep(100);
	return (EXIT_SUCCESS);
}

int		create_game(struct ipc *ipc)
{
	ipc->game = shmat(ipc->shm_id, NULL, 0);
	ipc->game->nb_players = 0;
	memset(ipc->game->players, -1, sizeof(ipc->game->players));
	memset(ipc->game->map, ' ', sizeof(ipc->game->map));
	return (EXIT_SUCCESS);
}

int		join_game(struct ipc *ipc)
{
	struct game		*game;
	struct player	*player;
	pid_t			pid;
	time_t			t;

	player = &ipc->player;
	game = ipc->game;
	pid = getpid();
	sem_lock(ipc->sem_id);
	size_t i = 0;
	while (i < MAX_PLAYERS && game->players[i] != -1)
		i++;
	if (i == MAX_PLAYERS)
	{
		sem_unlock(ipc->sem_id);
		dprintf(STDERR_FILENO, "Game is full\n");
		return (EXIT_FAILURE);
	}
	printf("nb_players\n");
	game->nb_players++;
	game->players[i] = pid;
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	srand((unsigned) time(&t));
	do {
		player->pos_x = rand() % WIDTH;
		player->pos_y = rand() % HEIGHT;
	}
	while (game->map[player->pos_y][player->pos_x] != ' ');
	game->map[player->pos_y][player->pos_x] = player->team + '0';
	sem_unlock(ipc->sem_id);
	return (play_game(ipc));
}

int		exit_game(struct ipc *ipc)
{
	sem_lock(ipc->sem_id);
	if (ipc->game->nb_players)
		ipc->game->nb_players--;
	ipc->game->map[ipc->player.pos_y][ipc->player.pos_x] = ' ';
	if (!ipc->game->nb_players)
	{
		shmdt(ipc->game);
		semctl(ipc->sem_id, IPC_RMID, 0);
		shmctl(ipc->shm_id, IPC_RMID, 0);
		return (EXIT_SUCCESS);
	}
	shmdt(ipc->game);
	sem_unlock(ipc->sem_id);
	return (EXIT_SUCCESS);
}

#include "lemipc.h"

int		play_game(struct ipc *ipc)
{
	recv_msg(ipc);
	show_game(ipc);
	sleep(100);
	return (EXIT_SUCCESS);
}

int		setup_chatbox(struct ipc *ipc)
{
	size_t chatbox_size = (CHAT_HEIGHT - 1) * ((WIDTH * 2) - 1) * sizeof(uint8_t);
	ipc->chatbox = malloc(chatbox_size);	
	if (!ipc->chatbox)
	{
		dprintf(STDERR_FILENO, "%s: malloc(): %s\n", PRG_NAME, strerror(errno));
		shmdt(ipc->game);
		return (EXIT_FAILURE);
	}
	memset(ipc->chatbox, ' ', chatbox_size);
	return (EXIT_SUCCESS);
}

int		create_game(struct ipc *ipc)
{
	ipc->game = shmat(ipc->shm_id, NULL, 0);
	ipc->game->nb_players = 0;
	memset(ipc->game->players, -1, sizeof(ipc->game->players));
	memset(ipc->game->map, ' ', sizeof(ipc->game->map));
	ipc->game->player_turn.team = 0;
	ipc->game->player_turn.pos_x = -1;
	ipc->game->player_turn.pos_y = -1;
	return (EXIT_SUCCESS);
}

int		join_game(struct ipc *ipc)
{
	char			buf[256];
	struct game		*game;
	struct player	*player;
	pid_t			pid;
	time_t			t;

	player = &ipc->player;
	game = ipc->game;
	pid = getpid();
	sem_lock(ipc->sem_id[PLAYERS]);
	size_t i = 0;
	while (i < MAX_PLAYERS && game->players[i] != -1)
		i++;
	if (i == MAX_PLAYERS)
	{
		sem_unlock(ipc->sem_id[PLAYERS]);
		dprintf(STDERR_FILENO, "Game is full\n");
		return (EXIT_FAILURE);
	}
	game->nb_players++;
	sem_unlock(ipc->sem_id[PLAYERS]);
	sem_lock(ipc->sem_id[MAP]);
	game->players[i] = pid;
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	srand((unsigned) time(&t));
	do {
		player->pos_x = rand() % WIDTH;
		player->pos_y = rand() % HEIGHT;
	}
	while (game->map[player->pos_y][player->pos_x] != ' ');
	game->map[player->pos_y][player->pos_x] = player->team + '0';
	sem_unlock(ipc->sem_id[MAP]);
	sprintf(buf, "Player joined team %d", player->team);
	send_msg_broadcast(ipc, buf, strlen(buf));
	return (play_game(ipc));
}

int		exit_game(struct ipc *ipc)
{
	sem_lock(ipc->sem_id[PLAYERS]);
	if (ipc->game->nb_players)
		ipc->game->nb_players--;
	int nb_players = ipc->game->nb_players;
	sem_unlock(ipc->sem_id[PLAYERS]);
	sem_lock(ipc->sem_id[MAP]);
	ipc->game->map[ipc->player.pos_y][ipc->player.pos_x] = ' ';
	if (!nb_players)
	{
		shmdt(ipc->game);
		msgctl(ipc->mq_id, IPC_RMID, 0);
		semctl(ipc->sem_id[MAP], IPC_RMID, 0);
		semctl(ipc->sem_id[PLAYERS], IPC_RMID, 0);
		semctl(ipc->sem_id[PLAY], IPC_RMID, 0);
		shmctl(ipc->shm_id, IPC_RMID, 0);
		return (EXIT_SUCCESS);
	}
	sem_unlock(ipc->sem_id[MAP]);
	shmdt(ipc->game);
	free(ipc->chatbox);
	return (EXIT_SUCCESS);
}

#include "lemipc.h"

int		count_nb_teams(struct ipc *ipc)
{
	int				sum;
	int				team_nb[10];
	struct game		*game;

	memset(team_nb, 0, sizeof(team_nb));
	game = ipc->game;
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (game->players[i].pid != -1)
			team_nb[game->players[i].team]++;
	}
	for (size_t i = 0; i < 10; i++)
	{
		if (team_nb[i])
			sum++;
	}
	return (sum);
}

int		play_game(struct ipc *ipc)
{
	int nb_teams;

	sem_lock(ipc->sem_id[PLAYERS]);
	do {
		recv_msg(ipc);
		show_game(ipc);
		sem_unlock(ipc->sem_id[PLAYERS]);
		sleep(1);
		sem_lock(ipc->sem_id[PLAYERS]);
	}
	while (ipc->game->nb_players < 4 || (nb_teams = count_nb_teams(ipc)) < 2);
	sem_unlock(ipc->sem_id[PLAYERS]);
	sem_lock(ipc->sem_id[MAP]);
	if (sem_trylock(ipc->sem_id[PLAY]) == EXIT_SUCCESS)
		printf("LOCKED !!!!\n");
	sem_unlock(ipc->sem_id[MAP]);
	sleep(1);
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
	ipc->game->player_turn = NULL;
	return (EXIT_SUCCESS);
}

int		join_game(struct ipc *ipc)
{
	char			buf[256];
	struct game		*game;
	struct player	*player;
	time_t			t;

	player = &ipc->player;
	game = ipc->game;
	player->pid = getpid();
	sem_lock(ipc->sem_id[PLAYERS]);
	size_t i = 0;
	while (i < MAX_PLAYERS && game->players[i].pid != -1)
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
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	srand((unsigned) time(&t));
	do {
		player->pos_x = rand() % WIDTH;
		player->pos_y = rand() % HEIGHT;
	}
	while (game->map[player->pos_y][player->pos_x] != ' ');
	game->map[player->pos_y][player->pos_x] = player->team + '0';
	memcpy(&game->players[i], player, sizeof(struct player));
	sem_unlock(ipc->sem_id[MAP]);
	sprintf(buf, "Player joined team %d", player->team);
	send_msg_broadcast(ipc, buf, strlen(buf));
	return (play_game(ipc));
}

int		exit_game(struct ipc *ipc)
{
	char			buffer[256];
	struct game		*game;
	pid_t pid;

	game = ipc->game;
	pid = getpid();
	sem_lock(ipc->sem_id[PLAYERS]);
	if (ipc->game->nb_players)
		ipc->game->nb_players--;
	int nb_players = ipc->game->nb_players;
	size_t i = 0;
	while (i < MAX_PLAYERS && game->players[i].pid != pid)
		i++;
	if (i != MAX_PLAYERS)
		game->players[i].pid = -1;
	sem_unlock(ipc->sem_id[PLAYERS]);
	sprintf(buffer, "Player left team %d", ipc->player.team);
	send_msg_broadcast(ipc, buffer, strlen(buffer));
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

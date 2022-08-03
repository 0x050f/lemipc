#include "lemipc.h"

int			create_game(int fd)
{
	printf("Creating the game...\n");
	g_lemipc.size = align_up(sizeof(t_game), getpagesize());
	if (ftruncate(fd, g_lemipc.size) < 0)
	{
		dprintf(STDERR_FILENO, "%s: ftruncate(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	g_lemipc.addr = mmap(0, g_lemipc.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (g_lemipc.addr == MAP_FAILED)
	{
		dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	/* Init mem */
	memset(g_lemipc.addr, 0, g_lemipc.size);
	/* Init map */
	t_game *game = g_lemipc.addr;
	if (sem_init(&game->sem_game, 1, 1) < 0)
	{
		dprintf(STDERR_FILENO, "%s: sem_init(): %s\n", PRG_NAME, strerror(errno));
		munmap(g_lemipc.addr, g_lemipc.size);
		return (EXIT_FAILURE);
	}
	if (sem_wait(&game->sem_game) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	game->nb_players = 0;
	game->pos_x_turn = -1;
	game->pos_y_turn = -1;
	for (size_t y = 0; y < HEIGHT; y++)
	{
		for (size_t x = 0; x < WIDTH; x++)
			game->map[y][x] = ' ';
	}
	if (sem_post(&game->sem_game) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	return (g_lemipc.size);
}

void		show_map(t_game *game)
{
	printf("\033[2J\033[H"); /* clear screen + top */
	printf("x: %d - y: %d\n", g_lemipc.pos_x, g_lemipc.pos_y);
	for (size_t x = 0; x <= WIDTH * 2; x++)
	{
		if (x == 0)
			printf("┌");
		else if (x == WIDTH * 2)
			printf("┐");
		else if (!(x % 2))
			printf("┬");
		else
			printf("─");
	}
	printf("\n");
	for (size_t y = 0; y < HEIGHT; y++)
	{
		printf("│");
		for (size_t x = 0; x < WIDTH; x++)
		{
			if (x == (size_t)g_lemipc.pos_x && y == (size_t)g_lemipc.pos_y)
				printf("\e[31m", game->map[y][x]);
			if (x == (size_t)game->pos_x_turn && y == (size_t)game->pos_y_turn)
				printf("\e[5m", game->map[y][x]);
			printf("%c\e[0m│", game->map[y][x]);
		}
		printf("\n");
		for (size_t x = 0; x <= WIDTH * 2; x++)
		{
			if (x == 0)
			{
				if (y != HEIGHT - 1)
					printf("├");
				else
					printf("└");
			}
			else if (x == WIDTH * 2)
			{
				if (y != HEIGHT - 1)
					printf("┤");
				else
					printf("┘");
			}
			else if (!(x % 2))
			{
				if (y != HEIGHT - 1)
					printf("┼");
				else
					printf("┴");
			}
			else
				printf("─");
		}
		printf("\n");
	}
	printf("\n");
}

// TODO: param chat
void		show_chatbox(uint8_t *chatbox)
{
	for (size_t y = 0; y <= CHAT_HEIGHT; y++)
	{
		for (size_t x = 0; x <= WIDTH * 2; x++)
		{
			if (x == 0 && y == 0)
				printf("┌");
			else if (x == WIDTH * 2 && y == 0)
				printf("┐");
			else if (x == 0 && y == CHAT_HEIGHT)
				printf("└");
			else if (x == WIDTH * 2 && y == CHAT_HEIGHT)
				printf("┘");
			else if (x == 0 || x == WIDTH * 2)
				printf("│");
			else if (y == 0 || y == CHAT_HEIGHT)
				printf("─");
			else
				printf("%c", chatbox[(x - 1) + (y - 1) * ((WIDTH * 2) - 1)]);
		}
		printf("\n");
	}
}

void		show_game(t_game *game, uint8_t *chatbox)
{
	if (sem_wait(&game->sem_game) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	show_map(game);
	if (sem_post(&game->sem_game) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	show_chatbox(chatbox);
}


int			count_nb_team(t_game *game)
{
	int		teams[10];

	memset(teams, 0, sizeof(teams));
	for (size_t y = 0; y < HEIGHT; y++)
	{
		for (size_t x = 0; x < WIDTH; x++)
		{
			if (game->map[y][x] != ' ')
				teams[game->map[y][x] - '0']++;
		}
	}
	int sum = 0;
	for (size_t i = 0; i < 10; i++)
	{
		if (teams[i])
			sum++;
	}
	return (sum);
}

void		recv_msg(sigval_t sv)
{
	(void)sv;
	char *buffer = NULL;
	struct mq_attr attr;
	if (mq_getattr(g_lemipc.mq_fd, &attr) < 0)
	{
		dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
		goto end;
	}
	buffer = malloc(attr.mq_msgsize);
	if (!buffer)
	{
		dprintf(STDERR_FILENO, "%s: malloc(): %s\n", PRG_NAME, strerror(errno));
		goto end;
	}
	int nr = mq_receive(g_lemipc.mq_fd, buffer, attr.mq_msgsize, NULL);
	if (nr == -1)
	{
		dprintf(STDERR_FILENO, "%s: mq_receive(): %s\n", PRG_NAME, strerror(errno));
		goto end;
	}
	// TODO: put in chatbox and action if needed
	memcpy(g_lemipc.chatbox + g_lemipc.cursor, buffer, nr);
	g_lemipc.cursor += nr;
	size_t size_to_add = ((WIDTH * 2) - 1) - (nr % ((WIDTH * 2) - 1));
	memset(g_lemipc.chatbox + g_lemipc.cursor, ' ', size_to_add);
	g_lemipc.cursor += size_to_add;
	show_game(g_lemipc.addr, g_lemipc.chatbox);
	end:
		free(buffer);
		struct sigevent sev;
		sev.sigev_notify = SIGEV_THREAD;
		sev.sigev_notify_function = recv_msg;
		sev.sigev_notify_attributes = NULL;
		sev.sigev_value.sival_ptr = &g_lemipc.mq_fd;
		if (mq_notify(g_lemipc.mq_fd, &sev) == -1)
			dprintf(STDERR_FILENO, "%s: mq_notify(): %s\n", PRG_NAME, strerror(errno));
}

void		send_msg(int mq_fd, char *msg, size_t size)
{
	struct mq_attr attr;
	printf("abc\n");
	if (mq_getattr(mq_fd, &attr) < 0)
		dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
	printf("def\n");
	if (size > (size_t)attr.mq_msgsize)
	{
		send_msg(mq_fd, "Message too long!", 17);
		return ;
	}
	if (mq_send(mq_fd, msg, size, 0) < 0)
		dprintf(STDERR_FILENO, "%s: mq_send(): %s\n", PRG_NAME, strerror(errno));
}

int			join_game(int shm_fd, int mq_fd, size_t size, int team_number)
{
	(void)mq_fd;
	g_lemipc.size = size;
	if (!g_lemipc.addr)
	{
		g_lemipc.addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if (g_lemipc.addr == MAP_FAILED)
		{
			dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
			return (EXIT_FAILURE);
		}
	}
	g_lemipc.chatbox = malloc((CHAT_HEIGHT - 1) * ((WIDTH * 2) - 1) * sizeof(uint8_t));
	if (!g_lemipc.chatbox)
	{
		dprintf(STDERR_FILENO, "%s: malloc(): %s\n", PRG_NAME, strerror(errno));
		munmap(g_lemipc.addr, size);
		return (EXIT_FAILURE);
	}
	memset(g_lemipc.chatbox, 'a', (CHAT_HEIGHT - 1) * ((WIDTH * 2) - 1) * sizeof(uint8_t));
	t_game *game = g_lemipc.addr;

	printf("boop\n");
	struct sigevent sev;
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = recv_msg;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = &mq_fd;
	if (mq_notify(mq_fd, &sev) == -1)
		dprintf(STDERR_FILENO, "%s: mq_notify(): %s\n", PRG_NAME, strerror(errno));
//	exit_game(game, size);
//	mq_unlink("/"PRG_NAME);
//	shm_unlink(PRG_NAME);
//	exit(0);
	show_game(game, g_lemipc.chatbox);
	printf("Waiting to join the game...\n");
	if (sem_wait(&game->sem_game) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	game->nb_players++;
	char buf[256];
	sprintf(buf, "Player has joined team %d", team_number);
	send_msg(mq_fd, buf, strlen(buf));
	time_t t;

	srand((unsigned) time(&t));
	do {
		g_lemipc.pos_x = rand() % WIDTH;
		g_lemipc.pos_y = rand() % HEIGHT;
	}
	while (game->map[g_lemipc.pos_y][g_lemipc.pos_x] != ' ');
		game->map[g_lemipc.pos_y][g_lemipc.pos_x] = team_number + '0';
	while (count_nb_team(game) < 2 || game->nb_players < 2)
	{
		if (sem_post(&game->sem_game) < 0)
			dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
		show_game(game, g_lemipc.chatbox);
		printf("Waiting... (Min 2 teams and 4 players)\n");
		sleep(100);
		if (sem_wait(&game->sem_game) < 0)
			dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	}
	if (sem_post(&game->sem_game) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	show_game(game, g_lemipc.chatbox);
	if (sem_trywait(&game->sem_game) < 0)
		show_game(game, g_lemipc.chatbox);
	else
	{
		game->pos_x_turn = g_lemipc.pos_x;
		game->pos_y_turn = g_lemipc.pos_y;
		if (sem_post(&game->sem_game) < 0)
			dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
		if (sem_wait(&game->sem_game) < 0)
			dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
		printf("Your turn !\n");
		struct mq_attr attr;
		if (mq_getattr(mq_fd, &attr) < 0)
			dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
		char	buffer[attr.mq_msgsize];
		int		ret = read(STDIN_FILENO, buffer, sizeof(buffer));
		(void)ret;
		if (sem_post(&game->sem_game) < 0)
			dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	}
	/*
	struct mq_attr attr;
	if (mq_getattr(mq_fd, &attr) < 0)
		dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
	printf("mq_flags: %ld\n", attr.mq_flags);
	printf("mq_maxmsg: %ld\n", attr.mq_maxmsg);
	printf("mq_msgsize: %ld\n", attr.mq_msgsize);
	printf("mq_cur: %ld\n", attr.mq_curmsgs);
	int ret;
	char	buffer[attr.mq_msgsize];
	printf("before read\n");
	ret = read(STDIN_FILENO, buffer, sizeof(buffer));
	printf("after read\n");
	if (mq_send(mq_fd, buffer, ret, 0) < 0)
		dprintf(STDERR_FILENO, "%s: mq_send(): %s\n", PRG_NAME, strerror(errno));
	*/
	return (exit_game(game, size));
}

int		exit_game(t_game *game, size_t size)
{
	int ret = EXIT_SUCCESS;
	sem_post(&game->sem_game);
	if (sem_wait(&game->sem_game) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	if (game->nb_players == 1)
	{
		sem_destroy(&game->sem_game);
		ret = END;
	}
	else
	{
		game->nb_players--;
		game->map[g_lemipc.pos_y][g_lemipc.pos_x] = ' ';
		if (sem_post(&game->sem_game) < 0)
			dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	}
	free(g_lemipc.chatbox);
	if (munmap(game, size) < 0)
		dprintf(STDERR_FILENO, "%s: munmap(): %s\n", PRG_NAME, strerror(errno));
	return (ret);
}

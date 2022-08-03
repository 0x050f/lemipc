#include "lemipc.h"

t_lemipc	g_lemipc;

size_t		align_up(size_t size, size_t align) {
	return (size + (align - (size % align)));
}

int			create_game(int fd)
{
	printf("Creating the game...\n");
	size_t	size = size = align_up(sizeof(t_game), getpagesize());
	if (ftruncate(fd, size) < 0)
	{
		dprintf(STDERR_FILENO, "%s: ftruncate(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
	{
		dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	/* Init mem */
	memset(addr, 0, size);
	/* Init map */
	t_game *game = addr;
	if (sem_init(&game->sem_player, 1, 1) < 0)
	{
		dprintf(STDERR_FILENO, "%s: sem_init(): %s\n", PRG_NAME, strerror(errno));
		munmap(addr, size);
		return (EXIT_FAILURE);
	}
	if (sem_wait(&game->sem_player) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	if (sem_init(&game-> sem_map, 1, 1) < 0)
	{
		dprintf(STDERR_FILENO, "%s: sem_init(): %s\n", PRG_NAME, strerror(errno));
		sem_destroy(&game->sem_player);
		munmap(addr, size);
		return (EXIT_FAILURE);
	}
	if (sem_wait(&game->sem_map) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	game->nb_player = 0;
	game->x_playing = -1;
	game->y_playing = -1;
	for (size_t y = 0; y < HEIGHT; y++)
	{
		for (size_t x = 0; x < WIDTH; x++)
			game->map[y][x] = ' ';
	}
	if (sem_post(&game->sem_player) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	if (sem_post(&game->sem_map) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	return (size);
}

void		destroy_game(t_game *game)
{
	sem_destroy(&game->sem_player);
	sem_destroy(&game->sem_map);
}

void		show_map(t_game *game)
{
	printf("\033[2J\033[H"); /* clear screen + top */
	printf("x: %d - y: %d\n", g_lemipc.x, g_lemipc.y);
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
			if (x == (size_t)game->x_playing && y == (size_t)game->y_playing)
				printf("\e[5m", game->map[y][x]);
			if (x == (size_t)g_lemipc.x && y == (size_t)g_lemipc.y)
				printf("\e[31m", game->map[y][x]);
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
void		show_chatbox()
{
	for (size_t y = 0; y < CHAT_HEIGHT; y++)
	{
		for (size_t x = 0; x <= WIDTH * 2; x++)
		{
			if (x == 0 && y == 0)
				printf("┌");
			else if (x == WIDTH * 2 && y == 0)
				printf("┐");
			else if (x == 0 && y == CHAT_HEIGHT - 1)
				printf("└");
			else if (x == WIDTH * 2 && y == CHAT_HEIGHT - 1)
				printf("┘");
			else if (x == 0 || x == WIDTH * 2)
				printf("│");
			else if (y == 0 || y == CHAT_HEIGHT - 1)
				printf("─");
			else // TODO: PRINT CHAT
				printf(" ");
		}
		printf("\n");
	}
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

int			join_game(int shm_fd, int mq_fd, size_t size, int team_number)
{
	(void)mq_fd;
	void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (addr == MAP_FAILED)
	{
		dprintf(STDERR_FILENO, "%s: mmap(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	g_lemipc.addr = addr;
	t_game *game = addr;
/*
	destroy_game(game);
	mq_unlink("/"PRG_NAME);
	shm_unlink(PRG_NAME);
	exit(0);
*/
	show_map(game);
	show_chatbox();
	printf("Waiting to join the game...\n");
	if (sem_wait(&game->sem_player) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	game->nb_player++;
	if (sem_post(&game->sem_player) < 0)
		dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
	printf("Joined as player !\n");
	// TODO: show chat and map and enter in room

	if (sem_wait(&game->sem_map) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	printf("lol\n");
	time_t t;

	srand((unsigned) time(&t));
	do {
		g_lemipc.x = rand() % WIDTH;
		g_lemipc.y = rand() % HEIGHT;
	}
	while (game->map[g_lemipc.y][g_lemipc.x] != ' ');
	game->map[g_lemipc.y][g_lemipc.x] = team_number + '0';
	if (sem_post(&game->sem_map) < 0)
		dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
	while (count_nb_team(game) < 2 || game->nb_player < 4)
	{
		show_map(game);
		show_chatbox();
		printf("Waiting... (Min 2 teams and 4 players)\n");
		sleep(1);
	}
	show_map(game);
	show_chatbox();
	while (1)
	{
		struct timespec time;
		clock_gettime(CLOCK_REALTIME, &time);
		time.tv_sec += 1;
		if (sem_timedwait(&game->sem_map, &time) < 0)
		{
			show_map(game);
			show_chatbox();
		}
		else
		{
			printf("Your turn !\n");
			struct mq_attr attr;
			if (mq_getattr(mq_fd, &attr) < 0)
				dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
			char	buffer[attr.mq_msgsize];
			int		ret = read(STDIN_FILENO, buffer, sizeof(buffer));
			(void)ret;
			if (sem_post(&game->sem_map) < 0)
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
	}
	int ret = EXIT_SUCCESS;
	if (game->nb_player == 1)
		destroy_game(game);
	else
	{
		if (sem_wait(&game->sem_player) < 0)
			dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
		game->nb_player--;
		if (sem_post(&game->sem_player) < 0)
			dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
		ret = END;
	}
	if (munmap(addr, size) < 0)
		dprintf(STDERR_FILENO, "%s: munmap(): %s\n", PRG_NAME, strerror(errno));
	return (ret);
}

void		signal_handler(int signum)
{
	(void)signum;
	bool end = false;
	if (g_lemipc.addr)
	{
		t_game *game = g_lemipc.addr;
		if (game->nb_player == 1)
		{
			printf("DESTROY GAME\n");
			destroy_game(game);
			end = true;
		}
		else
		{
			if (sem_wait(&game->sem_player) < 0)
				dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
			game->nb_player--;
			if (sem_post(&game->sem_player) < 0)
				dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
			if (sem_wait(&game->sem_map) < 0)
				dprintf(STDERR_FILENO, "%s: sem_wait(): %s\n", PRG_NAME, strerror(errno));
			game->map[g_lemipc.y][g_lemipc.x] = ' ';
			if (sem_post(&game->sem_map) < 0)
				dprintf(STDERR_FILENO, "%s: sem_post(): %s\n", PRG_NAME, strerror(errno));
		}
	}
	if (g_lemipc.addr)
		munmap(g_lemipc.addr, g_lemipc.size);
	if (g_lemipc.shm_fd >= 0)
		close(g_lemipc.shm_fd);
	if (g_lemipc.mq_fd >= 0)
		mq_close(g_lemipc.mq_fd);
	if (end)
	{
		mq_unlink("/"PRG_NAME);
		shm_unlink(PRG_NAME);
	}
	printf("\b\bLeaving the game...\n");
	exit(EXIT_SUCCESS);
}

/* TODO:
void		msg_rcv(sigval_t sv)
{
	struct mq_attr attr;
	ssize_t nr;
	int mq_fd = *((mqd_t *) sv.sival_ptr);

	if (mq_getattr(mq_fd, &attr) == -1)
		dprintf(STDERR_FILENO, "%s: mq_getattr(): %s\n", PRG_NAME, strerror(errno));
	char buf[attr.mq_msgsize];
	nr = mq_receive(mq_fd, buf, attr.mq_msgsize, NULL);
	if (nr == -1)
		dprintf(STDERR_FILENO, "%s: mq_received(): %s\n", PRG_NAME, strerror(errno));
	printf("Read %zd bytes from MQ: %.*s\n", nr, nr, buf);
}
*/

int			lemipc(int team_number)
{
	int		ret;
	struct	stat sb;

	if (signal(SIGINT, signal_handler) == SIG_ERR)
	{
		dprintf(STDERR_FILENO, "%s: signal(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if ((g_lemipc.shm_fd = shm_open(PRG_NAME, O_CREAT | O_RDWR, 0644)) < 0)
	{
		dprintf(STDERR_FILENO, "%s: shm_open(): %s\n", PRG_NAME, strerror(errno));
		return (EXIT_FAILURE);
	}
	if ((g_lemipc.mq_fd = mq_open("/"PRG_NAME, O_CREAT | O_RDWR, 0644, NULL)) < 0)
	{
		dprintf(STDERR_FILENO, "%s: mq_open(): %s\n", PRG_NAME, strerror(errno));
		close(g_lemipc.shm_fd);
		return (EXIT_FAILURE);
	}
	/* TODO:
	struct sigevent sev;

	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = msg_rcv;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = &g_lemipc.mq_fd;
	if (mq_notify(g_lemipc.mq_fd, &sev) == -1)
	{
		dprintf(STDERR_FILENO, "%s: mq_notify(): %s\n", PRG_NAME, strerror(errno));
		close(g_lemipc.shm_fd);
		mq_close(g_lemipc.mq_fd);
		return (EXIT_FAILURE);
	}
	*/
	if (fstat(g_lemipc.shm_fd, &sb) < 0)
	{
		dprintf(STDERR_FILENO, "%s: fstat(): %s\n", PRG_NAME, strerror(errno));
		ret = EXIT_FAILURE;
		goto end;
	}
	if (!sb.st_size)
	{
		if ((ret = create_game(g_lemipc.shm_fd)) == EXIT_FAILURE)
			goto end;
		ret = join_game(g_lemipc.shm_fd, g_lemipc.mq_fd, ret, team_number);
	}
	else
		ret = join_game(g_lemipc.shm_fd, g_lemipc.mq_fd, sb.st_size, team_number);
end:
	mq_close(g_lemipc.mq_fd);
	close(g_lemipc.shm_fd);
	if (ret == END)
	{
		mq_unlink("/"PRG_NAME);
		shm_unlink(PRG_NAME);
		ret = EXIT_SUCCESS;
	}
	return (ret);
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
	int team_number = atoi(argv[1]);
	g_lemipc.shm_fd = -1;
	g_lemipc.mq_fd = -1;
	g_lemipc.size = 0;
	g_lemipc.addr = NULL;
	return (lemipc(team_number));
}

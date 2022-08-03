#include "lemipc.h"

t_lemipc	g_lemipc;

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

void		signal_handler(int signum)
{
	(void)signum;
	int end = EXIT_SUCCESS;
	if (g_lemipc.addr)
		end = exit_game(g_lemipc.addr, g_lemipc.size);
	if (g_lemipc.shm_fd >= 0)
		close(g_lemipc.shm_fd);
	if (g_lemipc.mq_fd >= 0)
		mq_close(g_lemipc.mq_fd);
	if (end == END)
	{
		mq_unlink("/"PRG_NAME);
		shm_unlink(PRG_NAME);
	}
	printf("\b\bLeaving the game...\n");
	exit(EXIT_SUCCESS);
}
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
	g_lemipc.chatbox = NULL;
	return (lemipc(team_number));
}

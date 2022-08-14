#include "lemipc.h"

void		recv_all_msg(struct ipc *ipc)
{
	int nb_players;

	sem_lock(ipc->sem_id[PLAYERS]);
	nb_players = ipc->game->nb_players;
	sem_unlock(ipc->sem_id[PLAYERS]);
	for (size_t i = 0; i < (size_t)nb_players - 1; i++)
		recv_msg(ipc, NULL);
}

void		recv_msg(struct ipc *ipc, char buff[256])
{
	int						nr;
	struct ipc_msgbuf		mbuf;

	if ((nr = msgrcv(ipc->mq_id, &mbuf, sizeof(mbuf) - sizeof(mbuf.mtype), ipc->player.pid, 0)) >= 0)
	{
		if (ipc->chatbox)
			append_msg_chatbox(ipc->chatbox, mbuf.mtext, nr - sizeof(mbuf.mpid));
		if (buff)
			memcpy(buff, mbuf.mtext, nr);
	}
	else
	{
		dprintf(STDERR_FILENO, "%s: msgrcv(): %s\n", PRG_NAME, strerror(errno));
		exit_game(ipc);
	}
}

void		send_msg_pid(struct ipc *ipc, pid_t pid, char *msg)
{
	int				size;
	struct ipc_msgbuf	mbuf;

	memset(&mbuf, 0, sizeof(mbuf));
	mbuf.mtype = pid;
	mbuf.mpid = ipc->player.pid;
	size = strlen(msg);
	memcpy(mbuf.mtext, msg, size);
	if (msgsnd(ipc->mq_id, &mbuf, size + sizeof(mbuf.mpid), 0) < 0)
	{
		dprintf(STDERR_FILENO, "%s: msgsnd(): %s\n", PRG_NAME, strerror(errno));
		exit_game(ipc);
	}
}

void		send_msg_self(struct ipc *ipc, char *msg)
{
	append_msg_chatbox(ipc->chatbox, msg, strlen(msg));
}

void		send_msg_team(struct ipc *ipc, char *msg)
{
	char			msg_team[260];
	int				total;

	total = 0;
	sprintf(msg_team, "%d: %s", ipc->player.team, msg);
	sem_lock(ipc->sem_id[PLAYERS]);
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (ipc->game->players[i].pid != -1 && ipc->game->players[i].pid != ipc->player.pid)
		{
			send_msg_pid(ipc, ipc->game->players[i].pid, msg_team);
			total++;
		}
		else if (ipc->game->players[i].pid == ipc->player.pid)
			send_msg_self(ipc, msg_team);
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
}

void		send_msg_broadcast(struct ipc *ipc, char *msg)
{
	int			total;

	total = 0;
	sem_lock(ipc->sem_id[PLAYERS]);
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (ipc->game->players[i].pid != -1 && ipc->game->players[i].pid != ipc->player.pid)
		{
			send_msg_pid(ipc, ipc->game->players[i].pid, msg);
			total++;
		}
		else if (ipc->game->players[i].pid == ipc->player.pid)
			send_msg_self(ipc, msg);
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
}

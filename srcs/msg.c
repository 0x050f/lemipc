#include "lemipc.h"

void		recv_msg(struct ipc *ipc)
{
	pid_t			pid;
	int				nr;
	struct msgbuf	msg;

	pid = getpid();
	if ((nr = msgrcv(ipc->mq_id, &msg, sizeof(msg.mtext), pid, MSG_NOERROR | IPC_NOWAIT)) >= 0)
		append_msg_chatbox(ipc->chatbox, msg.mtext, nr);
}

void		send_msg_self(struct ipc *ipc, char *msg, size_t size)
{
	struct msgbuf mbuf;
	char buf[256];
	pid_t pid;

	if (size > 256)
	{
		sprintf(buf, "Message too long (max 256 char)");
		send_msg_self(ipc, buf, strlen(buf));
		return ;
	}
	pid = getpid();
	mbuf.mtype = pid;
	memcpy(mbuf.mtext, msg, size);
	msgsnd(ipc->mq_id, &mbuf, size, IPC_NOWAIT);
}

void		send_msg_broadcast(struct ipc *ipc, char *msg, size_t size)
{
	struct msgbuf mbuf;
	char buf[256];

	if (size > 256)
	{
		sprintf(buf, "Message too long (max 256 char)");
		send_msg_self(ipc, buf, strlen(buf));
		return ;
	}
	memcpy(mbuf.mtext, msg, size);
	sem_lock(ipc->sem_id[PLAYERS]);
	for (size_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (ipc->game->players[i].pid != -1)
		{
			mbuf.mtype = ipc->game->players[i].pid;
			msgsnd(ipc->mq_id, &mbuf, size, IPC_NOWAIT);
		}
	}
	sem_unlock(ipc->sem_id[PLAYERS]);
}

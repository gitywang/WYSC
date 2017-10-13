/*					------------ 线程池调度管控 ------------
 *	利用线程池实现沙箱总线程管控，若沙箱总线程数大于线程池线程总数，暂停此时有事件的沙箱，
 *	再根据线程池空闲线程数唤醒某个处于暂停态的沙箱，若不存在可唤醒沙箱则发信号运行新沙箱，
 *	但是唤醒沙箱时处于暂停态的沙箱优先级高于未运行的沙箱；
 *	添加线程池的目的是：
 *		根据线程池的线程数量管控大规模时沙箱总线程数量，避免线程数过多时
 *		导致频繁地切换线程耗费系统资源，时刻保证线程池处于满载或接近满载运行状态。
 * *******************************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <signal.h>

#define	NAMESIZE					128
#define BUFSIZE						128
#define MAX_EVENTS					10000
#define MAX_FAILED_COUNT			10
#define POOL_THREAD_NUMBER			20
#define MAX_THREAD_NUMBER			5000
#define handle_error(msg)			do{perror(msg);exit(EXIT_FAILURE);}while(0)

/* 客户端状态 */
enum sandbox_state_en
{
	RUNNING=1,
	SUSPEND,
	WAITING,
	OVER,/* 链表头状态 */
};

/* 客户端与服务器用于传输数据的结构体 */
struct sandbox_message_st
{
	int sandbox_cmd;
	char container_name[NAMESIZE];//容器名
};

/* 任务队列结构体 */
struct task_list_st
{
	int connect_fd;
	struct sandbox_message_st sandbox_message;
	int sandbox_thread_number;
	enum sandbox_state_en sandbox_state;
	int resume_failed_count;
	struct task_list_st *prev;
	struct task_list_st *next;
};

/* 线程池结构体，多线程共享数据时应考虑数据同步问题 */
struct thread_pool_st 
{
	int fd;
	int shutdown;
	int total_thread_number;
	pthread_mutex_t mutex_fd;
	pthread_mutex_t mutex_number;
	pthread_cond_t cond_main;
	pthread_cond_t cond_equal;
	pthread_t *tid;
};


static int epfd;/* 主线程添加实例，对等线程删除实例 */
static struct epoll_event events[MAX_EVENTS];
static int queue_number;//用于记录从管道读取的数量
static int start_number;//用于记录待启动样本的数量,有值代表还有实际未启动的，为0代表全部启动
static const char fifo_1[] = "/opt/malware_sandbox_deploy/thread_pool/sh_to_pool";
static const char fifo_2[] = "/opt/malware_sandbox_deploy/thread_pool/pool_to_sh";
static int fifo_wfd = -1;//pool_to_sh写端fd
static int fifo_rfd = -1;//sh_to_pool读端fd
static struct thread_pool_st	*pool;
static struct task_list_st		*head;

void task_list_init(void);
void pool_init(void);
void handle_connection(void);
void *thread_function(void *);
void setnonblocking(int);
void add_task_list(int);
int delete_task_list(int);
void print_task_list(void);
int modify_task_list(int, struct sandbox_message_st);
struct task_list_st *find_suspend_sandbox(void);
struct task_list_st *find_resume_sandbox(int, int);
int min(int, int);
void suspend_sandbox(char *);
void resume_sandbox(char *);
void pool_destory(void);


void task_list_init(void)
{
	head = (struct task_list_st *)malloc(sizeof(struct task_list_st));
	if(head == NULL)
		handle_error("malloc->head");
	head->connect_fd = -1;
	memset(head->sandbox_message.container_name, '\0', NAMESIZE);
	head->sandbox_message.sandbox_cmd = -1;
	head->sandbox_thread_number = 0;
	head->sandbox_state = OVER;
	head->resume_failed_count = 0;

	head->prev = head;
	head->next = head;
}

void pool_init(void)
{
	int var;
	int err;
	pool = (struct thread_pool_st *)malloc(sizeof(struct thread_pool_st));
	if(pool == NULL)
		handle_error("malloc->pool");

	pool->fd = -1;
	pool->shutdown = 0;
	pool->total_thread_number = 0;
	pthread_mutex_init(&(pool->mutex_fd), NULL);
	pthread_mutex_init(&(pool->mutex_number), NULL);
	pthread_cond_init(&(pool->cond_main), NULL);
	pthread_cond_init(&(pool->cond_equal), NULL);

	pool->tid = (pthread_t *)malloc(sizeof(pthread_t) * POOL_THREAD_NUMBER);
	if(pool->tid == NULL)
		handle_error("malloc->pool->tid");
	
	for(var = 0; var < POOL_THREAD_NUMBER; var++)
	{
		err = pthread_create(&(pool->tid[var]), NULL, thread_function, NULL);//暂时未传参
		if(err)
			fprintf(stderr, "pool_init()->pthread_create: %s\n", strerror(errno));
	}
}

void pool_destory(void)
{
	int var;
	struct task_list_st *p1 = head->next;
	struct task_list_st *p2;

	if(pool->shutdown)
		return;
	pool->shutdown = -1;

	/* 唤醒所有等待线程，线程池要销毁了 */
	pthread_cond_broadcast(&(pool->cond_main));
	pthread_cond_broadcast(&(pool->cond_equal));

	/* 阻塞等待线程退出 */
	for(var = 0; var < POOL_THREAD_NUMBER; var++)
		pthread_join(pool->tid[var], NULL);
	
	/* 先释放结构体内指针，再释放结构体 */
	free(pool->tid);

	/* 销毁任务队列 */
	while(p1 != NULL)
	{
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}
	free(head);
	head = NULL;

	/* 销毁互斥量和条件变量 */
	pthread_mutex_destroy(&(pool->mutex_fd));
	pthread_mutex_destroy(&(pool->mutex_number));
	pthread_cond_destroy(&(pool->cond_main));
	pthread_cond_destroy(&(pool->cond_equal));

	/* 销毁线程池结构体 */
	free(pool);
	pool = NULL;

	/* 关闭socket */
	//close(listener);
}


/*	epoll通知某fd有事件，有数据等待接收的事件只会通知一次，因为客户端方send之后便会recv阻塞等待服务端send
 *	因此服务端第一次recv：会接收到实际数据；第二次recv：recv返回－1，表示无数据可接收
 *	但epoll仍然存在某fd两次有事件的情况，假如第一次epoll通知某fd有数据要接收，在未接收时该fd连接的客户端与服务端断开
 *  此时便会再通知一次相同fd有事件，此时recv就应该执行两次：一次recv数据，一次recv表示对端关闭
 * */

void *thread_function(void *p)
{
	struct sandbox_message_st recvbuf;
	struct sandbox_message_st sendbuf;
	struct task_list_st *task = NULL;
	int fd, ret;
	//struct epoll_event ev;

	while(1)
	{
		pthread_mutex_lock(&(pool->mutex_fd));
		
		while(pool->fd == -1)
			pthread_cond_wait(&(pool->cond_equal), &(pool->mutex_fd));
		
		fd = pool->fd;
		pool->fd = -1;

		pthread_cond_signal(&(pool->cond_main));

		pthread_mutex_unlock(&(pool->mutex_fd));
		while (1)
		{
			bzero(&recvbuf, sizeof(recvbuf));
			ret = recv(fd, &recvbuf, sizeof(recvbuf), 0);
			if(ret == -1)
			{
				/* cmd=2/3先发送，shutdown后发送，由于线程并发shutdown先处理，cmd=2/3后处理，导致send之前该客户端fd已经关闭了 */
				if(errno == EBADF)
					break;

				sendbuf.sandbox_cmd = 0;
				memset(sendbuf.container_name, '\0', NAMESIZE);
				/* 客户端send后会阻塞recv该消息，等不到该消息便不会再send，防止fd多次有事件,但实际却recv一次的情况 */
				if(send(fd, &sendbuf, sizeof(sendbuf), MSG_NOSIGNAL) == -1)/* MSG_NOSIGNAL */
				{
					//perror("thread_function()->send1");
					fprintf(stderr, "fd = %d send failed, reason : %s\n", fd, strerror(errno));
					if(errno == EPIPE)
						continue;
				}
				break;/* 循环recv时接收到无数据时便不再recv */
			}
			else if(ret == 0)/* 客户端与服务器断开连接 */
			{
				pthread_mutex_lock(&(pool->mutex_number));

				fprintf(stderr, "fd 为 %d 的客户端退出\n", fd);	
				/* 删除相应任务链表节点、更新总线程数、根据目前总线程数和待启动样本数量决定是否需要再唤醒或启动样本 */
				pool->total_thread_number -= delete_task_list(fd);
				/* 防止任务链表已经清空但是客户端总线程数并未归零的情况 */
				if(head->next == head)
				{
					fprintf(stderr, "Before clear out, pool->total_thread_number = %d\n", pool->total_thread_number);
					pool->total_thread_number = 0;
				}
				fprintf(stderr, "fd 为 %d 的客户端退出后 pool->total_thread_number = %d\n", fd, pool->total_thread_number);

				/* 有客户端退出，空缺出部分线程数量，可唤醒暂停态客户端或启动新客户端 */
				if((task = find_resume_sandbox(pool->total_thread_number, -2)) != NULL)
				{
					/* resume某客户端后更新总线程数 */
					pool->total_thread_number += task->sandbox_thread_number;
					fprintf(stderr, "某客户端退出，fd 为 %d 的客户端被唤醒后 pool->total_thread_number = %d\n", task->connect_fd, pool->total_thread_number);
					/* resume某客户端，激活客户端时处于暂停态客户端的优先级高于未运行客户端*/
					task->sandbox_state = RUNNING;
					resume_sandbox(task->sandbox_message.container_name);
				}

				//pthread_mutex_unlock(&(pool->mutex_number));

				/* 客户端退出后删除相应客户端的epoll实例 */
				if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
					perror("thread_function()->epoll_ctl");
				fprintf(stderr, "删除epoll 中fd 为 %d 的客户端实例成功\n", fd);
				close(fd);/* 若没有显式地关闭fd，主线程添加实例和对等线程删除实例不会产生冲突 */
				
				pthread_mutex_unlock(&(pool->mutex_number));/* epoll删除实例操作也应该在锁内完成 */
				break;/* 循环recv时对方断开连接便不再recv */
			}
			else/* recv成功且有数据，维护任务链表内容 */
			{
				pthread_mutex_lock(&(pool->mutex_number));

				fprintf(stderr, "recvbuf.sandbox_cmd = %d\n", recvbuf.sandbox_cmd);
				if(recvbuf.sandbox_cmd == 1)
					fprintf(stderr, "recvbuf.container_name = %s\n", recvbuf.container_name);
				pool->total_thread_number += modify_task_list(fd, recvbuf);
				fprintf(stderr, "自增或自减后 pool->total_thread_number = %d\n", pool->total_thread_number);

				/* 长安提议：a客户端线程数＋1后大于MAX_THREAD_NUMBER，此时就应该暂停最大线程数量的客户端 */
				if(pool->total_thread_number > MAX_THREAD_NUMBER)
				{
					/* 获取suspend客户端信息 */
					if((task = find_suspend_sandbox()) != NULL)
					{
						/* 暂停某客户端后更新总线程数 */
						pool->total_thread_number -= task->sandbox_thread_number; 
						//fprintf(stderr, "线程数量超限，fd 为 %d 的客户端被暂停后 pool->total_thread_number = %d\n", task->connect_fd, pool->total_thread_number);
						fprintf(stderr, "线程数量超限，最大线程数量 %d ,fd 为 %d 的客户端被暂停后 pool->total_thread_number = %d\n", task->sandbox_thread_number, task->connect_fd, pool->total_thread_number);
						/* 暂停某客户端,暂停目前最大线程数量的客户端 */
						printf("freeze sandbox : %s\n", task->sandbox_message.container_name);
						task->sandbox_state = SUSPEND;
						suspend_sandbox(task->sandbox_message.container_name);

						/* 获取resume客户端信息 */
						if((task = find_resume_sandbox(pool->total_thread_number, fd)) != NULL)
						{
							/* resume某客户端后更新总线程数 */
							pool->total_thread_number += task->sandbox_thread_number;
							fprintf(stderr, "线程数量超限暂停某客户端后，fd 为 %d 的客户端被唤醒 pool->total_thread_number = %d\n", task->connect_fd, pool->total_thread_number);
							/* resume某客户端，激活客户端时处于暂停态客户端的优先级高于未运行客户端*/
							task->sandbox_state = RUNNING;
							resume_sandbox(task->sandbox_message.container_name);
						}
					}
				}/* 若客户端总线程数量自减后，空闲线程数量可再容纳一个客户端线程数量，唤醒某客户端或启动新客户端 */
				else if((MAX_THREAD_NUMBER - pool->total_thread_number - (start_number<<4))>>4 > 0)
				{
					//fprintf(stderr, "(MAX_THREAD_NUMBER - pool->total_thread_number - (start_number<<4))>>4 = %d\n", (MAX_THREAD_NUMBER - pool->total_thread_number - (start_number<<4))>>4);
					if((task = find_resume_sandbox(pool->total_thread_number, -2)) != NULL)
					{
						/* resume某客户端后更新总线程数 */
						pool->total_thread_number += task->sandbox_thread_number;
						fprintf(stderr, "线程数量空缺，fd 为 %d 的客户端被唤醒后 pool->total_thread_number = %d\n", task->connect_fd, pool->total_thread_number);
						/* resume某客户端，激活客户端时处于暂停态客户端的优先级高于未运行客户端*/
						task->sandbox_state = RUNNING;
						resume_sandbox(task->sandbox_message.container_name);
					}
				}

				pthread_mutex_unlock(&(pool->mutex_number));
			}
		}
	}

	pthread_exit(NULL);/* 此句应该是不可达的 */
}


void setnonblocking(int fd)
{
	int flags;
	if((flags = fcntl(fd, F_GETFL, 0)) == -1)
		handle_error("fcntl1");
	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		handle_error("fcntl2");
}

void print_task_list(void)
{
	if(!head) return ;
	struct task_list_st *p = head->next;
	while(p != head)
	{
		fprintf(stderr, "p->connect_fd = %d\n", p->connect_fd);
		fprintf(stderr, "p->sandbox_message.container_name = %s\n", p->sandbox_message.container_name);
		fprintf(stderr, "p->sandbox_thread_number = %d\n", p->sandbox_thread_number);
		fprintf(stderr, "p->sandbox_state = %d\n", p->sandbox_state);
		//fprintf(stderr, "p->resume_failed_count = %d\n", p->resume_failed_count);
		fprintf(stderr, "---------------------------------------\n");
		
		p = p->next;
	}
	fprintf(stderr, "=====================================================\n");
}


void add_task_list(int fd)
{
	if(!head) return;
	struct task_list_st *new;
	new = (struct task_list_st *)malloc(sizeof(struct task_list_st));
	if(new == NULL)
		handle_error("add_task_list()->malloc");
	new->connect_fd = fd;
	memset(new->sandbox_message.container_name, '\0', NAMESIZE);
	new->sandbox_message.sandbox_cmd = -1;
	new->sandbox_thread_number = 0;//自加
	new->sandbox_state = WAITING;//WAITING
	new->resume_failed_count = 0;

	/* 数据后插 */
	new->next = head;
	new->prev = head->prev;
	new->prev->next = new;
	new->next->prev = new;
}

int delete_task_list(int fd)
{
	int delete_thread_number = 0;

	if(!head) return 0;
	struct task_list_st *p = head->next;
	while(p != head)
	{
		if(p->connect_fd == fd)
		{
			delete_thread_number = p->sandbox_thread_number;
			p->prev->next = p->next;
			p->next->prev = p->prev;
			free(p);
			if(p->sandbox_state == SUSPEND)/* 暂停态客户端pool->total_thread_number已经减去了暂停态客户端线程数 */
				return 0;
			return delete_thread_number;
		}
		p = p->next;
	}
	return 0;
}

/*	modify_task_list
 *	根据recv接收的数据信息，维护客户端线程数量，修改客户端任务状态
 *	返回值：
 *		线程启动		  1
 *		线程退出		－1
 *		未找到			  0
 * */
int modify_task_list(int fd, struct sandbox_message_st buf)
{
	if(!head) return 0;
	struct task_list_st *p = head->next;
	while(p != head)
	{
		if(p->connect_fd == fd)
		{
			switch(buf.sandbox_cmd)
			{
				case 1:
					p->sandbox_state = RUNNING;//WAITING --> RUNNING，cmd=1仅会发送一次
					strncpy(p->sandbox_message.container_name, buf.container_name, NAMESIZE);
					break;
				case 2:
					p->sandbox_thread_number++;//自加
					fprintf(stderr, "fd 为 %d 的客户端线程数自增\n", fd);
					if(p->sandbox_state == SUSPEND)//已经处于暂停态的客户端仅更新链表节点信息，不再更新pool->total_thread_number
						return 0;
					return 1;
				case 3:
					p->sandbox_thread_number--;//自减
					fprintf(stderr, "fd 为 %d 的客户端线程数自减\n", fd);
					if(p->sandbox_state == SUSPEND)//已经处于暂停态的客户端仅更新链表节点信息，不再更新pool->total_thread_number
						return 0;
					return -1;
			}
		}
		p = p->next;
	}
	return 0;
}


struct task_list_st *find_suspend_sandbox(void)
{
	if(!head) return NULL;

	struct task_list_st *p = head->next;
	struct task_list_st *node = NULL;
	
	while(p != head)
	{
		if(p->sandbox_state == RUNNING)
		{
			if(node == NULL)
				node = p;
			else if(p->sandbox_thread_number > node->sandbox_thread_number)
				node = p;
		}
		p = p->next;
	}

	return node;
}

/*	find_resume_sandbox
 *	长安提议：根据超时时间和线程数量，计算出最理想被继续执行的客户端
 *	返回值
 *		需要继续执行的任务节点			p	
 * */
struct task_list_st *find_resume_sandbox(int thread_number, int fd)
{
	if(!head) return NULL;
	int idle_thread_number = MAX_THREAD_NUMBER - thread_number;
	struct task_list_st *p = head->next;
	
	while(p != head)
	{
		if((p->sandbox_state == SUSPEND) && (p->connect_fd != fd))
		{
			if(p->sandbox_thread_number < idle_thread_number)
			{
				//p->sandbox_state = RUNNING;//SUSPEND --> RUNNING，是否应该真正重新启动成功后再修改状态？
				p->resume_failed_count = 0;
				return p;
			}
			else/* 某suspend客户端线程数>=线程池空闲线程数，不唤醒该客户端，唤醒失败计数+1，若所有suspend客户端均不满足唤醒条件，则向线程池中添加一个新的任务 */
			{
				p->resume_failed_count++;
				/* 若某suspend态客户端唤醒失败计数达到最大值，则即使线程池空闲线程数不满足唤醒条件也不再启动新客户端 */
				if(p->resume_failed_count > MAX_FAILED_COUNT)
					return NULL;
			}
		}
		p = p->next;
	}
	
	/* 除刚暂停的客户端外不存在suspend态客户端，或所有suspend态客户端均不满足唤醒条件，且等待队列中有任务，则启动新客户端 */

	if(queue_number > 0)/* 等待队列中有任务 */
	{
		int num = (MAX_THREAD_NUMBER - pool->total_thread_number - (start_number<<4))>>4;/* 可启动的样本数量 */
		if((num > 0))
		{
			char s[32] = {'\0'};
			int min_num = min(num, queue_number);
			start_number += min_num;
			sprintf(s, "%d+", min_num);
			if(write(fifo_wfd, s, strlen(s)) == -1)
				handle_error("handle_connection()->write");
			fprintf(stderr, "向pool_to_sh 的管道的写端写入 %s 成功\n", s);
			/* 更新待启动样本数量 */
			queue_number -= min_num;
		}
	}

	return NULL;
}

int min(int a, int b)
{
	if(a < b)
		return a;
	return b;
}


/*	suspend_sandbox
 *	根据客户端号暂停客户端
 * */
void suspend_sandbox(char container_name[])
{
	/* 冻结命令 lxc-freeze -n container_name */
	char command[128] = {'\0'};
	sprintf(command, "lxc-freeze -n %s", container_name);
	printf("command = %s\n", command);
	if(system(command) == -1)
		handle_error("suspend_sandbox()->system");
	
}

/*	resume_sandbox
 *	根据客户端号唤醒客户端
 * */
void resume_sandbox(char container_name[])
{
	/* 解冻命令 lxc-unfreeze -n container_name */
	char command[128] = {'\0'};
	sprintf(command, "lxc-unfreeze -n %s", container_name);
	printf("command = %s\n", command);
	if(system(command) == -1)
		handle_error("resume_sandbox()->system");
}

void handle_connection(void)
{
	int listener, new_fd, /* epfd ,*/ nfds, n;
	int backlog = 128;
	struct sockaddr_un seraddr, cliaddr;
	struct epoll_event  ev /*, events[MAX_EVENTS]*/;
	socklen_t cliaddr_len = sizeof(cliaddr);
	char buf[BUFSIZE];


	/* socket */
	if((listener = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		handle_error("socket");
	fprintf(stderr, "socket 创建成功\n");

	setnonblocking(listener);/* 将fd设为非阻塞模式 */

	/* bind */
	bzero(&seraddr, sizeof(seraddr));
	seraddr.sun_family = AF_UNIX;
	strcpy(seraddr.sun_path, "/tmp/thread_pool");
	unlink("/tmp/thread_pool");/* 防止重复绑定地址时bind失败 */
	if(bind(listener, (struct sockaddr *)&seraddr, sizeof(seraddr)) == -1)
		handle_error("bind");
	fprintf(stderr, "bind 成功\n");

	/* listen */
	if(listen(listener, backlog) == -1)
		handle_error("listen");
	fprintf(stderr, "listen 成功\n");
	
//	/* epoll events */
//	if((epfd = epoll_create(MAX_EVENTS)) == -1)
//		handle_error("epoll_create");

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listener;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev) == -1)
		handle_error("epoll_ctl");
	fprintf(stderr, "监听socket 加入epoll 成功\n");

	while(1)
	{
		fprintf(stderr, "等待事件的到来epoll_wait \n");
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if(nfds == -1)
			perror("epoll_wait");

		for(n = 0; n < nfds; n++)
		{
			if(events[n].data.fd == listener)//处理连接
			{
				fprintf(stderr, "有客户端连接服务端, before accept\n");
				while(1)/* 多个客户端同时连接listener，epoll仅会告诉内核有事件,而并不明确相同事件有几次，若只accept一次，其他待连接的fd仍在队列中等待连接 */
				{
					new_fd = accept(listener, (struct sockaddr *)&cliaddr, &cliaddr_len);
					if(new_fd == -1)
					{
						if(errno == EAGAIN)
							break;
					}
					fprintf(stderr, "有客户端连接服务端，分配的fd 为 %d \n", new_fd);

					setnonblocking(new_fd);/* 将fd设为非阻塞模式 */
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = new_fd;
					if(epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &ev) == -1)
						perror("epoll_ctl");
					fprintf(stderr, "将新连接的fd 为 %d 的客户端实例加入epoll 成功\n", new_fd);

					/* 添加任务链表节点，任务状态初始为WAITING态，首次连接时发送信息包含容器名称 */
					add_task_list(new_fd);
					fprintf(stderr, "将fd 为 %d 的客户端信息添加到任务链表成功\n", new_fd);
					start_number--;
					if(start_number < 0)
						start_number = 0;
				}
			}
			else if(events[n].data.fd == fifo_rfd)
			{
				fprintf(stderr, "shell_to_pool 管道的读端 %d 有事件\n", fifo_rfd);
				if(read(fifo_rfd, buf, BUFSIZE) == -1)
					perror("handle_connection()->read");
				else if(buf[0] == '+')//有待启动样本
				{
					queue_number = atoi(buf+1);/* 获取待启动样本数量 */
						
					if(queue_number > 0)
					{
						/* 查询pool->total_thread_number数量，根据计算得出的可启动样本数量写管道 */
						pthread_mutex_lock(&(pool->mutex_number));
						int num = (MAX_THREAD_NUMBER - pool->total_thread_number - (start_number<<4))>>4;
						pthread_mutex_unlock(&(pool->mutex_number));

						if(num > 0)
						{
							char s[32] = {'\0'};
							int min_num = min(num, queue_number);
							start_number += min_num;
							sprintf(s, "%d+", min_num);
							if(write(fifo_wfd, s, strlen(s)) == -1)
								perror("handle_connection()->write");
							fprintf(stderr, "向pool_to_sh 的管道的写端写入 %s 成功\n", s);
							/* 更新待启动样本数量 */
							queue_number -= min_num;
						}
					}
				}
			}
			else
			{
				fprintf(stderr, "连接服务端fd 为 %d 的客户端有事件\n", events[n].data.fd);
				pthread_mutex_lock(&(pool->mutex_fd));

				while(pool->fd != -1)
					pthread_cond_wait(&(pool->cond_main), &(pool->mutex_fd));
				
				pool->fd = events[n].data.fd;
				pthread_cond_signal(&(pool->cond_equal));
				
				pthread_mutex_unlock(&(pool->mutex_fd));
			/*	注意：着重考虑当线程池中工作线程满时，待处理的的fd多次有事件而处理该fd时仅recv一次的情况 */
			}
		}
	}
}


void create_epoll(void)
{
	/* epoll events */
	if((epfd = epoll_create(MAX_EVENTS)) == -1)
		handle_error("epoll_create");
	fprintf(stderr, "创建epoll 成功\n");
}

void open_fifo(void)
{
	struct epoll_event ev;

	/* 打开shell_to_poll读端，并将此读端fd加入epoll进行监听 */
	if(access(fifo_1, F_OK) == -1)
		if(mkfifo(fifo_1, 0777) == -1)
			handle_error("open_fifo()->mkfifo fifo_1");

	if(access(fifo_2, F_OK) == -1)
		if(mkfifo(fifo_2, 0777) == -1)
			handle_error("open_fifo()->mkfifo fifo_2");

	fifo_rfd = open(fifo_1, O_RDONLY | O_NONBLOCK);
	if(fifo_rfd == -1)
		handle_error("open()->fifo_rfd");
	fprintf(stderr, "打开fifo_rfd 成功\n");

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = fifo_rfd;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, fifo_rfd, &ev) == -1)
		handle_error("open_fifo()->epoll_ctl");
	fprintf(stderr, "将fifo_rfd 加入 epoll 成功\n");

	/* 打开poll_to_shell写端 */
	//fifo_wfd = open(fifo_2, O_WRONLY | O_NONBLOCK);
	fifo_wfd = open(fifo_2, O_RDWR | O_NONBLOCK);
	if(fifo_wfd == -1)
		handle_error("open()->fifo_wfd");
	fprintf(stderr, "打开fifo_wfd 成功\n");
}

int main()
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGPIPE);
	sa.sa_flags = 0;

	if(sigaction(SIGPIPE, &sa, NULL) == -1)
		handle_error("sigaction()");

	pool_init();
	task_list_init();

	create_epoll();
	open_fifo();

	handle_connection();

	
	//pool_destory();

	return 0;
}

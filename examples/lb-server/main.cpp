#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <event.h>
#include <syslog.h>


#define PORT        25341
#define BACKLOG     5
#define MEM_SIZE    1024

struct sock_ev {
    struct event* read_ev;
    struct event* write_ev;
    char* buffer;
};



static struct event_base* base;

static unsigned int port = 10000;
static unsigned int ip = 0x7f000001;
static bool foreground = false;

static struct option long_options[] = {
    {"help",  no_argument, 0,  'h' },
    {"port",    required_argument, 0,  'p' },
    {"ip",      required_argument, 0,  'i' },
    {"foreground",    no_argument, 0,  'f' },
    {0,         0,                 0,  0 }
};

static void help(void) {
	printf("Usage: lb-server\n");
	printf("	-h, --help       display this help and exit\n");
	printf("	-i, --ip         setting listen ip address\n");
	printf("	-p, --port       setting listen port\n");
	printf("	-f, --foreground running in foreground\n");
}

static bool parser_opt(int argc, char **argv) {
    int c;
    int digit_optind = 0;
	
    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        c = getopt_long(argc, argv, "hp:i:f",
                 long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'p':
        	port = atoi(optarg);
            break;

        case 'i':
        	ip = (unsigned int)inet_addr(optarg);
            break;

        case 'f':
        	foreground = true;
            break;

        default:
            help();
            return false;
        }
    }
    return true;
}


void release_sock_event(struct sock_ev* ev)
{
    event_del(ev->read_ev);
    free(ev->read_ev);
    free(ev->write_ev);
    free(ev->buffer);
    free(ev);
}


void on_write(int sock, short event, void* arg)
{
    char* buffer = (char*)arg;
    
    send(sock, buffer, strlen(buffer), 0);

    free(buffer);
}


void on_read(int sock, short event, void* arg)
{
    struct event* write_ev;
    int size;
    struct sock_ev* ev = (struct sock_ev*)arg;
    
    /* 准备接收缓冲区 */
    ev->buffer = (char*)malloc(MEM_SIZE);
    if (ev->buffer == NULL) {
    	printf("no memory\n");
    	return;
    }
    bzero(ev->buffer, MEM_SIZE);
    
    /* 接收Socket数据 */
    size = recv(sock, ev->buffer, MEM_SIZE, 0);
    if (size <= 0) {
    	if (size == 0) {
    		/* 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
    	} else {
			/* 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
    	}
        release_sock_event(ev);
        close(sock);
        return;
    }
    
    /* 处理数据 */
    ev->buffer[256] = '\0';
    syslog(LOG_INFO, "GET QUESTION: %s\n", ev->buffer);
    free(ev->buffer);
    
    /* 应答 */
#if 0    
    event_set(ev->write_ev, sock, EV_WRITE, on_write, ev->buffer);
    if (event_base_set(base, ev->write_ev) < 0) {
    	printf("event_base_set error\n");
    	return;
    }
    if (event_add(ev->write_ev, NULL) < 0) {
    	printf("event_add error\n");
    	return;
    }
#endif
}


void on_accept(int sock, short event, void* arg)
{
    struct sockaddr_in cli_addr;
    int newfd;
    socklen_t sin_size;
    struct sock_ev* ev = (struct sock_ev*)malloc(sizeof(struct sock_ev));
    if (!ev) {
    	printf("no memory\n");
    	return;
    }
    ev->read_ev = (struct event*)malloc(sizeof(struct event));
    ev->write_ev = (struct event*)malloc(sizeof(struct event));
    if (!ev->read_ev || !ev->write_ev) {
    	printf("no memory\n");
    	return;
    }
    
    sin_size = sizeof(struct sockaddr_in);
    newfd = accept(sock, (struct sockaddr*)&cli_addr, &sin_size);
    if (newfd < 0) {
    	perror("accept error");
    	return;
    }
    
    event_set(ev->read_ev, newfd, EV_READ|EV_PERSIST, on_read, ev);
    if (event_base_set(base, ev->read_ev) < 0) {
    	printf("event_base_set error\n");
    	return;
    }
    if (event_add(ev->read_ev, NULL) < 0) {
    	printf("event_add error\n");
    	return;
    }
}


int main(int argc, char* argv[]) {
    struct sockaddr_in my_addr;
    int sock;
    int yes;
    
    /* 解析参数 */
    if (!parser_opt(argc, argv)) {
    	return 0;
    }
    if (!port || (port <= 1024) || (port >= 0x10000)) {
    	printf("Invalid port number\n");
    	return 0;
    }

	/* 打开端口 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
    	perror("socket error\n");
    	goto error1;
    }
    yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    	perror("setsockopt error");
    	goto error2;
    }
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) < 0) {
    	perror("bind error");
    	goto error2;
    }
    if (listen(sock, BACKLOG) < 0) {
    	perror("listen error");
    	goto error2;
    }
	
    openlog("lb-server", LOG_ODELAY | LOG_PID, LOG_USER);

	/* 处理Socket事件 */
    struct event listen_ev;
    base = event_base_new();
    if (base == NULL) {
    	printf("event_base_new error\n");
    	goto error2;
    }
    event_set(&listen_ev, sock, EV_READ|EV_PERSIST, on_accept, NULL);
    if (event_base_set(base, &listen_ev) < 0) {
    	printf("event_base_set error\n");
    	goto error3;
    }
    if (event_add(&listen_ev, NULL) < 0) {
    	printf("event_base_set error\n");
    	goto error3;
    }
    event_base_dispatch(base);

	return 0;
error3:
	event_base_free(base);
error2:
	close(sock);
error1:
	return -1;
}


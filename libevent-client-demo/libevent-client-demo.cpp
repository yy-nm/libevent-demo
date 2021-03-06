// libevent-client-demo.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"


#include <fcntl.h>

#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <sys/types.h>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>


#ifdef _WIN32
#pragma comment (lib, "Ws2_32.lib")
#endif // _WIN32


typedef intptr_t ssize_t;

std::mutex g_lock;
std::string buffcontent;

static void
echo_read_cb(struct bufferevent *bev, void *ctx)
{
	char buff[1024];
	size_t buff_len = sizeof(buff);

	memset(buff, 0, buff_len);

	struct evbuffer *input = bufferevent_get_input(bev);
	size_t sz = evbuffer_get_length(input);
	
	size_t readsz = 0;
	size_t totalsz = 0;
	while (totalsz < sz) {
		readsz = evbuffer_remove(input, buff, buff_len);
		totalsz += readsz;
		std::cout << std::string(buff, readsz) << std::endl;
	}
}

static void
echo_write_cb(struct bufferevent *bev, void *ctx)
{
	struct evbuffer *output = bufferevent_get_output(bev);

	intptr_t fd = bufferevent_getfd(bev);

	size_t totalsz = evbuffer_get_length(output);
	size_t writesz;
	while (totalsz > 0) {
		writesz = evbuffer_write(output, fd);
		if (writesz < 0) {
			return;
		}

		totalsz -= writesz;
	}
}

void thread_func(struct bufferevent *bev)
{
	std::string content;
	while (true) {
		std::cin >> content;
		std::lock_guard<std::mutex> g(g_lock);
		buffcontent += content;
	}
}

static void
echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
	}
	else if (events & BEV_EVENT_CONNECTED) {
		bufferevent_setcb(bev, echo_read_cb, echo_write_cb, echo_event_cb, NULL);
		bufferevent_enable(bev, EV_READ | EV_WRITE);

		std::thread t(std::bind(thread_func, bev));
		t.detach();
	}
}

void
run(void)
{
	evutil_socket_t fd;
	struct sockaddr_in sin;
	struct event_base *base;

	base = event_base_new();
	if (!base) {
		printf("event base new fail\n");
		return; /*XXXerr*/
	}
		

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0x7f000001);;
	sin.sin_port = htons(9876);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	evutil_make_socket_nonblocking(fd);

	
	struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, NULL, NULL, echo_event_cb, NULL);

	if (bufferevent_socket_connect(bev, (sockaddr *)&sin, sizeof(sin)) < 0) {
		bufferevent_free(bev);
		printf("connect fail\n");
		return;
	}

	while (!event_base_loop(base, EVLOOP_ONCE | EVLOOP_NONBLOCK)) {
		std::lock_guard<std::mutex> g(g_lock);

		struct evbuffer *output = bufferevent_get_output(bev);
		evbuffer_add(output, buffcontent.data(), buffcontent.size());
		echo_write_cb(bev, NULL);
		buffcontent.clear();
	}
}

void InitSocket()
{
#ifdef _WIN32
	WSADATA wsaData;
	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(-1);
	}
#endif
}

void ClearSocket()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

int
main(int c, char **v)
{
	InitSocket();

	setvbuf(stdout, NULL, _IONBF, 0);

	run();

	ClearSocket();
	return 0;
}

// libevent-server-demo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#pragma comment (lib, "Ws2_32.lib")
#endif // _WIN32

static void
echo_read_cb(struct bufferevent *bev, void *ctx)
{
	/* This callback is invoked when there is data to read on bev. */
	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

	/* Copy all the data from the input buffer to the output buffer. */
	evbuffer_add_buffer(output, input);
}

static void
echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
	}
}

static void
accept_conn_cb(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *address, int socklen,
	void *ctx)
{
	/* We got a new connection! Set up a bufferevent for it. */
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(
		base, fd, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);

	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

static void
accept_error_cb(struct evconnlistener *listener, void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
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
main(int argc, char **argv)
{
	struct event_base *base;
	struct evconnlistener *listener;
	struct sockaddr_in sin;

	int port = 9876;
	int sin_len = sizeof(sin);

	InitSocket();

	if (evutil_parse_sockaddr_port("0.0.0.0:9876", (struct sockaddr *)&sin, &sin_len) == -1) {
		puts("parse ip and port error");
		ClearSocket();
		return 1;
	}


	base = event_base_new();
	if (!base) {
		puts("Couldn't open event base");
		ClearSocket();
		return 1;
	}

	/* Clear the sockaddr before using it, in case there are extra
	* platform-specific fields that can mess us up. */
	//memset(&sin, 0, sizeof(sin));
	/* This is an INET address */
	//sin.sin_family = AF_INET;
	/* Listen on 0.0.0.0 */
	//sin.sin_addr.s_addr = htonl(0);
	/* Listen on the given port. */
	//sin.sin_port = htons(port);

	listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
		(struct sockaddr*)&sin, sizeof(sin));
	if (!listener) {
		perror("Couldn't create listener");
		ClearSocket();
		return 1;
	}
	evconnlistener_set_error_cb(listener, accept_error_cb);

	//event_base_dispatch(base);
	while (!event_base_loop(base, EVLOOP_ONCE | EVLOOP_NONBLOCK)) {
		
	}

	ClearSocket();
	return 0;
}
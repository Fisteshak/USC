#include "UServer.h"
#include <iostream>
#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

//���� ���� ������ ��� ������ �� ������
static void echo_read_cb(struct bufferevent *bev, void *ctx)
{
    /* This callback is invoked when there is data to read on bev. */
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);


    //��������� � ������� ������ �� ������
    size_t len = evbuffer_get_length(input);
    void *data;

    data = malloc(len);    
    evbuffer_copyout(input, data, len);    
    
    for (int i = 0; i < len; i++)
        std::cout << *(static_cast<char*>(data)+i);

    std::cout << std::endl;

    free(data);

    /* Copy all the data from the input buffer to the output buffer. */
    evbuffer_add_buffer(output, input);
}
//��� ������� ��� ������
static void echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_ERROR)
        perror("Error from bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
        bufferevent_free(bev);
    }
}

//��� ����������� ������ �������
static void accept_conn_cb(evconnlistener *listener, evutil_socket_t fd,
                           sockaddr *address, int socklen, void *ctx)
{
    //�������� base �� listener
    event_base *base = evconnlistener_get_base(listener);
    //������� ����� �����-����������
    bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    //���������� ����������� ��� ������ � �������
    bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);

    bufferevent_enable(bev, EV_READ | EV_WRITE);
}
//��� ������ �� ��������� ������
static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. "
                    "Shutting down.\n",
            err, evutil_socket_error_to_string(err));
    //��������� ���� �������
    event_base_loopexit(base, NULL);
}

int main(int argc, char **argv)
{
    initWinsock();

    int port = 9554;

    //��������� ��������� �� ���������
    //���� ��, �� ���������� ����
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }
    if (port <= 0 || port > 65535)
    {
        puts("Invalid port");
        return 1;
    }

    struct event_base *base;         //��������� �������
    struct evconnlistener *listener; //����������� �����
    struct sockaddr_in list_inf;     //���������� ��� listener

    base = event_base_new();

    ZeroMemory(&list_inf, sizeof(list_inf));

    in_addr serv_ip;

    inet_pton(AF_INET, "127.0.0.1", &serv_ip);

    list_inf.sin_family = AF_INET;
    list_inf.sin_addr = serv_ip;
    list_inf.sin_port = htons(port);
    //���������� �����-���������
    listener = evconnlistener_new_bind(base, accept_conn_cb,
                                       NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                       -1, (sockaddr *)&list_inf, sizeof(list_inf));

    if (!listener)
    {
        perror("Could not create listener");
        return 1;
    }
    //���������� ���������� ������ �� ������-���������
    evconnlistener_set_error_cb(listener, accept_error_cb);

    //��������� ���� �������
    event_base_dispatch(base);
    std::cout << "server finished working";
    return 0;
}

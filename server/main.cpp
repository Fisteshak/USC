#include "UServer.h"
#include <iostream>
#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

using std::endl, std::cin, std::cout;

//если есть данные для чтения из буфера
static void echo_read_cb(struct bufferevent *bev, void *ctx)
{
    
    /* This callback is invoked when there is data to read on bev. */
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);


    //прочитать и вывести данные из буфера
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
//при событии или ошибке
static void echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_ERROR) {  
        if (EVUTIL_SOCKET_ERROR() == 10054) 
            cout << "Client has disconnected!" << endl;
        else 
            cout  << "Error has happened" << endl;
    }

    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
        bufferevent_free(bev);      //закрыть сокет буферевента
        
    }

}

//при подключении нового клиента
static void accept_conn_cb(evconnlistener *listener, evutil_socket_t fd,
                           sockaddr *address, int socklen, void *ctx)
{
    //получить base от listener
    event_base *base = evconnlistener_get_base(listener);
    //создать новый сокет-буферевент
    bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    //установить обработчики для чтения и события
    bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);

    bufferevent_enable(bev, EV_READ | EV_WRITE);

    cout << "CLient has connected" << endl;
}
//при ошибке на слушающем сокете
static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. "
                    "Shutting down.\n",
            err, evutil_socket_error_to_string(err));
    //завершить цикл событий
    event_base_loopexit(base, NULL);
}

int main(int argc, char **argv)
{

    if (!initWinsock()) {
        perror("Failed to initialize Winsock");
        return 1;        
    }

    int port = 9554;

    //проверить передеаны ли аргументы
    //если да, то установить порт
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }
    if (port <= 0 || port > 65535)
    {
        puts("Invalid port");
        return 1;
    }

    struct event_base *base;         //структура событий
    struct evconnlistener *listener; //принимающий сокет
    struct sockaddr_in list_inf;     //информация для listener

    base = event_base_new();

    ZeroMemory(&list_inf, sizeof(list_inf));

    in_addr serv_ip;

    inet_pton(AF_INET, "127.0.0.1", &serv_ip);

    list_inf.sin_family = AF_INET;
    list_inf.sin_addr = serv_ip;
    list_inf.sin_port = htons(port);
    //установить сокет-слушатель
    listener = evconnlistener_new_bind(base, accept_conn_cb,
                                       NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                       -1, (sockaddr *)&list_inf, sizeof(list_inf));

    if (!listener)
    {
        perror("Could not create listener");
        return 1;
    }
    //установить обработчик ошибок на сокете-слушателе
    evconnlistener_set_error_cb(listener, accept_error_cb);

    //завершить цикл событий
    event_base_dispatch(base);
    
    std::cout << "server finished working";
    return 0;
}

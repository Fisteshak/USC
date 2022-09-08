#include "UClient.h"
#include <iostream>
#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

#include <thread>

using std::cout, std::cin, std::endl;

void event_cb(struct bufferevent *bev, short events, void *ptr)
{
    if (events & BEV_EVENT_CONNECTED) {
         /* We're connected to 127.0.0.1   Ordinarily we'd do
            something here, like start reading or writing. */
        bufferevent_write(bev, "Hello, world!", 3);
        std::cout << "Connected" << std::endl;
    }
    
    if (events & BEV_EVENT_ERROR) {  
        if (EVUTIL_SOCKET_ERROR() == 10054) 
            cout << "Server stopped working!" << endl;
        else 
            cout  << "Error has happened" << endl;
    }

    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
        bufferevent_free(bev);      //завершить цикл событий в случае ошибки
    } 

}

void write_cb(struct bufferevent *bev, void *ctx)
{
    cout << 'Data was written' << endl;
}



void libev_start(event_base *base)
{
    event_base_dispatch(base);    
}

int main()
{
    int port = 9554;
    
    struct event_base *base;         //структура событий
    struct bufferevent *bev;        //сокет для передачи данных
    struct sockaddr_in cl_inf;     //информация для listener    

    if (!initWinsock()) {
        perror("Failed to initialize Winsock");
        return 1;        
    }

    base = event_base_new();

    ZeroMemory(&cl_inf, sizeof(cl_inf));
    in_addr serv_ip;

    inet_pton(AF_INET, "127.0.0.1", &serv_ip);

    cl_inf.sin_family = AF_INET;
    cl_inf.sin_addr = serv_ip;
    cl_inf.sin_port = htons(port);

    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, NULL, write_cb, event_cb, NULL);  

    if (bufferevent_socket_connect(bev,
        (struct sockaddr *)&cl_inf, sizeof(cl_inf)) < 0) {
        /* Error starting connection */
        std::cout << "Can't connect to server!" << std::endl;
        bufferevent_free(bev);
        return -1;
    }

    bufferevent_enable(bev, EV_READ | EV_WRITE);

    
    std::thread libevthr(libev_start, base);    

    std::string s;
    do
    {
        cin >> s;
    } while(s != "xxx");
    
    libevthr.join();        
    std::cout << "client finished working";
    return 0;

}
#include "UServer.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <iostream>
#include <string>
#include <atomic>
#include <thread>

UServer::UServer(int listenerPort, std::string listenerIP, uint32_t max_connections) : listenerPort(listenerPort), listenerIP(listenerIP)
{
    if (max_connections == 0) {
        throw std::runtime_error("max_connections value should not be zero");
    }
    if (max_connections > SOMAXCONN) {
        throw std::runtime_error("max_connections value is too big");
    }
    this->max_connections = max_connections;
    return;
}

UServer::~UServer()
{
    cleanupWinsock();
    return;

}
//Инициализирует сетевой интерфейс для сокетов.
//Возвращает true в случае успеха, false в случае неудачи.
bool UServer::initWinsock()  
{
    WSADATA wsaData;
    WORD ver = MAKEWORD(2, 2); //версия Winsock
    if (WSAStartup(ver, &wsaData) != 0)
    {        
        return false;
    }
    return true;
}

//закрывает интерфейсы winsock
void UServer::cleanupWinsock()   
{
    WSACleanup();
    return;

}

// создает сокет-слушатель    
SOCKET UServer::createListener() 
{

    //создать сокет	
    SOCKET listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) return INVALID_SOCKET;

    //позволяет системе использовать только что закрытое соединение
    //(например в случае быстрого перезапуска программы)
	int on = 1;
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0)
	{		
		closesocket(listener);		
		return INVALID_SOCKET;
	}

    //перевести сокет в неблокирующий режим
    DWORD nonBlocking = 1;
    if (ioctlsocket(listener, FIONBIO, &nonBlocking) < 0)
    {    
        closesocket(listener);
		return INVALID_SOCKET;            
    }

    //преобразовать IP в структуру in_addr 
    in_addr serv_ip;

	if (inet_pton(AF_INET, listenerIP.data(), &serv_ip) <= 0) {
        closesocket(listener);
		return INVALID_SOCKET;
	}

	//привязка к сокету адреса и порта
	sockaddr_in servInfo;

	ZeroMemory(&servInfo, sizeof(servInfo));    //обнулить

	servInfo.sin_family = AF_INET;
	servInfo.sin_port = htons(listenerPort);
	servInfo.sin_addr = serv_ip;

	int servInfoLen = sizeof(servInfo);

	if (bind(listener, (sockaddr*)&servInfo, servInfoLen) != 0) {
		closesocket(listener);
		cleanupWinsock();	
		return INVALID_SOCKET;
	}

    
    if (listen(listener, SOMAXCONN) < 0)
    {
        closesocket(listener);
		cleanupWinsock();	
		return INVALID_SOCKET;    
    }

    return listener;
}
void UServer::stop()
{
    if (_status != status::stopped) {

        _status = status::stopped;

        closesocket(listener);
        for (int i = 1; i < fds.size(); i++) {
            closesocket(fds[i].fd);
        }
        fds.clear();

        joinThreads();
    }
    return;
}

void UServer::joinThreads()
{
	handlingLoopThread.join();
    return;
}

uint32_t UServer::get_block_size()
{
	return block_size;
}

void UServer::set_block_size(uint32_t size)
{
	block_size = size;
    return;
}



void UServer::handlingLoop()
{	
	while (_status == status::up) {	
        uint32_t events_num;    //количество полученных событий от сокетов
		events_num = WSAPoll(fds.data(), fds.size(), -1);   //ждать входящих событий		

        if (events_num == SOCKET_ERROR) {
            _status = status::error_wsapoll;
            return;
        }
        
        int handled_events = 0;

        //если на слушателе есть событие, то это входящее соединение        
        if (fds[0].revents == POLLRDNORM) {    
            
            while (true) {
                SOCKET new_conn = accept(listener, NULL, NULL);  //принять соединение
                if (new_conn == INVALID_SOCKET) {   //если accept возвратил INVALID_SOCKET, то все соединения приняты
                    int err = WSAGetLastError();
                    if (WSAGetLastError() != WSAEWOULDBLOCK) { _status = status::error_accept_connection; }                                        
                    break;                    
                }
                if (conn_handler) {
                        conn_handler();
                }
                fds.push_back({new_conn, POLLIN, 0});     //добавить в массив
            }         
            handled_events++;
            //fds[0].revents = 0;
        }

        if (_status != status::up) break;

		//перебрать все сокеты до тех пор пока не обработаем все полученные события
        data_buffer_t data_buf(block_size);		//буфер для данных
        for (int i = 1; handled_events < events_num; i++) {
            
            //if (fds[i].revents | POLLIN)       //если есть входящие данные
            if (fds[i].revents != 0)       //если есть входящие данные            
            
            {
                int recieved_data;		//количество прочитанных байт
				recieved_data = recv(fds[i].fd, data_buf.data(), block_size, 0);  //прочитать                
               
                //если мы успешно прочитали данные
                if (recieved_data > 0) {    
                    if (data_handler) {
                        data_handler(data_buf);  //вызвать обработчик
                    }                    
                }
                
                //при нормальном закрытии соединения
                if (recieved_data == 0) {                    
                    if (disconn_handler) {
                        disconn_handler();
                    }
                    closesocket(fds[i].fd);
                    fds.erase(fds.begin()+i);      //удалить из массива соединений                    
                }

                //если recv вернул ошибку
                if (recieved_data == SOCKET_ERROR) {  
                    //при "жестком" закрытии соединения
                    if (WSAGetLastError() == WSAECONNRESET) {
                        if (disconn_handler) {
                            disconn_handler();
                        }    
                    }
                    closesocket(fds[i].fd);
                    fds.erase(fds.begin()+i);      //удалить из массива соединений  
                }

                handled_events++;
                
            }
        }	
	}
	return;
}


//запускает сервер (цикл приема данных)
UServer::status UServer::run()
{
    if (_status == status::up) {
        stop();
    }
        
	_status = status::up;	
    
	//инициализировать Winsock
	if (!initWinsock()) {
		return _status = status::error_winsock_init;
	}

    SOCKET listener = createListener();  //создать сокет-слушатель

    fds.push_back({listener, POLLIN, 0});  //поместить слушателя в начало массива дескрипторов сокетов    
    this->listener = listener;

	if (listener == INVALID_SOCKET) {
		std::cout << "create listener error" << std::endl;
		return _status = error_listener_create;
	}	
	//запустить поток обработки входящих сообщений
	handlingLoopThread = std::thread(&handlingLoop, this);
    //handlingLoop();

	return _status;
}

void UServer::set_data_handler(data_handler_t handler)
{
    data_handler = handler;
    return;
}

void UServer::set_conn_handler(conn_handler_t handler)
{
    conn_handler = handler;
    return;

}

void UServer::set_disconn_handler(conn_handler_t handler)
{
    disconn_handler = handler;
    return;

}



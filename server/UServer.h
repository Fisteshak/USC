#pragma once
#include <string>
class UServer
{
private:
    /* data */
public:
    UServer();
    ~UServer();

};

bool initWinsock();  //Инициализирует сетевой интерфейс для сокетов.
                     //Возвращает true в случае успеха, false в случае неудачи.


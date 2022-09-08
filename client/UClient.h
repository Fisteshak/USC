#pragma once

class UClient
{
private:
    /* data */
public:

    UClient(
        /* args */);
    ~UClient();
};

bool initWinsock();  //Инициализирует сетевой интерфейс для сокетов.
                     //Возвращает true в случае успеха, false в случае неудачи.

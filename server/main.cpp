#include "UServer.h"
#include <memory>
#include <thread>

void data_cb(UServer::data_buffer_t& data)
{
    
}


int main()
{
    
    UServer server(8159, "127.0.0.1", 50);
    server.set_data_handler(data_cb);
    server.run();
    // //server.stop();        
}

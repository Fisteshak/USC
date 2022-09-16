#include "UServer.h"
#include <memory>
#include <thread>
int main()
{

    //std::unique_ptr <UServer> server(new UServer(8159, "127.0.0.1", 50));
    UServer server(8159, "127.0.0.1", 50);
    

}

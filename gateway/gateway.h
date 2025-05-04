#include <crow.h>

class GatewayServer {
public:
    void run(uint16_t port);

private:
    std::string generate_uuid();
};

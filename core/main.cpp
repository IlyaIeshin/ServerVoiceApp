#include "gateway/gateway.h"

int main() {
    GatewayServer server;
    server.run(18080);
    return 0;
}

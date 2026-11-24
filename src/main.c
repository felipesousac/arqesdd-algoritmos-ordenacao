#include "server.h"

int main(int argc, char **argv) {
  const char *address = argc > 1 ? argv[1] : SERVER_DEFAULT_ADDRESS;
  
  return server_run(address);
}

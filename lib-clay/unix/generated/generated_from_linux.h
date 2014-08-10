#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <signal.h>
#include <dirent.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#define __USE_GNU 
#include <dlfcn.h>
#undef __USE_GNU

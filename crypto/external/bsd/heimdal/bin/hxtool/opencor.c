#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_mysyscall 462

int main() {
    long int ret = syscall(SYS_mysyscall);
    if (ret == 0) {

    exit_t;
    } else {
    
success_t;
  }
    return 0;
}


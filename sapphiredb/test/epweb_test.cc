// kqueue test
// send port 19998
// recv port 19999
#include "common/kqueue.h"
#include "common/net.h"
#include "common/thread_pool.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <string.h>
#include <stdio.h>

void cat(int client, FILE *resource)
{
    //::std::cout << "***cat***" << ::std::endl;
    char buf[1024];

    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

void headers(int client, const char *filename)
{
    //::std::cout << "***headers***" << ::std::endl;
     char buf[1024];
     (void)filename;  /* could use filename to determine file type */
     strcpy(buf, "HTTP/1.0 200 OK\r\n");
     send(client, buf, strlen(buf), 0);
     strcpy(buf, "Server: cdcupt's Server\r\n");
     send(client, buf, strlen(buf), 0);
     sprintf(buf, "Content-Type: text/html\r\n");
     send(client, buf, strlen(buf), 0);
     strcpy(buf, "\r\n");
     send(client, buf, strlen(buf), 0);
}
void not_found(int client)
{
    //::std::cout << "***notfound***" << ::std::endl;
     char buf[1024];
     FILE *resource = NULL;
     sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
     send(client, buf, strlen(buf), 0);
     sprintf(buf, "Server: cdcupt's Server\r\n");
     send(client, buf, strlen(buf), 0);
     sprintf(buf, "Content-Type: text/html\r\n");
     send(client, buf, strlen(buf), 0);
     sprintf(buf, "\r\n");
     send(client, buf, strlen(buf), 0);
     resource = fopen("../webdocs/404notfound.html", "r");
     cat(client, resource);
     fclose(resource);
}


void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;

    resource = fopen(filename, "r");
    if (resource == NULL){
        //send(client,not_found,sizeof(not_found),0);
        not_found(client);
    }
    else
    {
        //http_send(client, filename);
        //send(client,headers,sizeof(headers),0);
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}

int main(){
    sapphiredb::common::Kqueue* kque = new sapphiredb::common::Kqueue("127.0.0.1", 19999, sapphiredb::common::Netcon::IPV4, 1000, 1024, 20);
    kque->listenp();
    sapphiredb::common::ThreadPool epoll_loop(1);
    epoll_loop.enqueue([&](){
        while(1){
            kque->loop_once(1000);
        }
    });

    sapphiredb::common::ThreadPool do_something(4);
    do_something.enqueue([&](){
        while(1){
            kque->doSomething([&](int32_t fd){
                //::std::cout << *(kque->getData()->buf) << ::std::endl;
                serve_file(fd, "../test/webdocs/index.html");
            });
        }
    });
    
    while(1){
        sleep(1);
    }

    return 0;
}

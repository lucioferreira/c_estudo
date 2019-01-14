/*
    C ECHO client example using sockets
*/
#include<stdio.h> //printf
#include<stdlib.h>
#include <unistd.h>
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char server_reply[2000];

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("erro ao criar o socket");
    }
    puts("Socket criado");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("falha ao conectar. Erro");
        return 1;
    }

    puts("Connected\n");

  memset(&server_reply, 0, sizeof(server_reply) );
  char *message = "{\"mensagem\":\"teste de mensagem\"}";
  printf("Enviando json: %s\n", message);

  //envia o json
  if( send(sock , message , strlen(message) , 0) < 0) {
      puts("falha no envio");
      return 1;
  }

  //Receive a reply from the server
  if( recv(sock , server_reply , 2000 , 0) < 0) {
      puts("recv falhou");
  }

  puts("resposta do Server: ");
  puts(server_reply);
  puts("---  fim  ---");

 close(sock);
 
 return 0;
}
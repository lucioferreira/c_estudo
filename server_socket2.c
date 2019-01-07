/*
    C socket server simples
    aceita mais de um client
    loop eterno
    cria um processo separado com fork para cada conexão
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write

void dostuff(int); /* function prototype */

int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c , pid;
    struct sockaddr_in server , client;
    int port = 8888;

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("Erro ao criar o socket\n");
    }
    puts("Socket criado com sucesso");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind falhou. Erro");
        return 1;
    }
    printf("bind ok na porta %d\n", port);

    //Listen
    listen(socket_desc , 3);

    //Accept and incoming connection
    puts("Esperando por novas conexões...");
    c = sizeof(struct sockaddr_in);
    
    //loop para receber todos as conexões que entram
    while(1) { 

	    //accept connection from an incoming client
	    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	    if (client_sock < 0) {
	        perror("accept failed");
	        continue;
	    }
	    puts("Connection accepted");
	    
	    pid = fork();
	    if(pid < 0) {
			perror("Erro no fork!!");
			exit(1);	    
	    }
	    
	    if(pid == 0){
			close(socket_desc);
			dostuff(client_sock);
			exit(0);	    
	    }
	    else {
	    	close(client_sock);
	    }
    
    } /* while(1) */

    return 0;
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
   int read_size;
   char buffer[2048];
   memset( &buffer, 0, sizeof(buffer));
	//Receive a message from client
	while( (read_size = read(sock , buffer, sizeof(buffer))) > 0 ) {
	  //Send the message back to client
	  write(sock , buffer , strlen(buffer));
	  printf("mensagem: %s\n", buffer);
	  memset( &buffer, 0, sizeof(buffer));
	 }
	
	 if(read_size == 0) {
	  puts("Client disconnected");
	  fflush(stdout);
	 }
	 else if(read_size == -1) {
	  perror("recv failed");
	  exit(1);
	 }   
}

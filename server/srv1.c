/*
    C socket server com json e sqlite3 integrado
    aceita mais de um client
    loop eterno
    cria um processo separado com fork para cada conexão
    TODO: integração com json e sqlite3
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write

void connection_proxy(int); /* função de entrada para cada conexão */
void char *get_comando(char *json); /* retorna o campo "comando" do json */

int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c , pid;
    struct sockaddr_in server , client;
    int port = 8888;

    //cria o socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("Erro ao criar o socket\n");
    }
    puts("Socket criado com sucesso");

    //prepara o sockaddr_na structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );

    // Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind falhou. Erro");
        return 1;
    }
    printf("bind ok na porta %d\n", port);

    // Listen
    listen(socket_desc , 3);

    //Aceita as conexões que estão entrando
    puts("Esperando por novas conexões...");
    c = sizeof(struct sockaddr_in);
    
    //loop para receber todos as conexões que entram
    while(1) { 

	    //aceita a conexão de um client
	    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	    if (client_sock < 0) {
	        perror("accept falhou!!");
	        continue;
	    }
	    puts("Conexão foi aceita!!!");
	    
	    pid = fork();
	    if(pid < 0) {
			perror("Erro no fork!!");
			exit(1);	    
	    }
	    
	    if(pid == 0){
			close(socket_desc);
			connection_proxy(client_sock);
			exit(0);	    
	    }
	    else {
	    	close(client_sock);
	    }
    
    } /* while(1) */

    return 0;
}

/******** connection_proxy() *********************
 Existe uma instancia em separado dessa função 
 para cada conexão.  Ela trabalha com toda a 
 comunicação quando uma conexão é estabelecida.
 *************************************************/
void connection_proxy (int sock)
{
   int read_size;
   char buffer[2048];
   char comando[50];
   memset( &buffer, 0, sizeof(buffer));
   memset( &comando, 0, sizeof(comando));
	//Receive a message from client
	while( (read_size = read(sock , buffer, sizeof(buffer))) > 0 ) {
		
		comando = get_comando(buffer);
		puts(comando);
	  	
	  	//Envia a mensagem mensagem de volta ao client
	  	write(sock , buffer , strlen(buffer));
	  	printf("mensagem: %s\n", buffer);

	  	memset( &buffer, 0, sizeof(buffer));
	  	memset( &comando, 0, sizeof(comando));

	 }
	
	 if(read_size == 0) {
	  puts("Client disconectou");
	  fflush(stdout);
	 }
	 else if(read_size == -1) {
	  perror("recv falhou");
	  exit(1);
	 }   
}

/******** *get_comando() *********************
 Retorna o conteúdo do campo "comando" 
 enviado dentro do objeto json.
 *********************************************/

void char *get_comando(char *json) {


	return NULL;

}

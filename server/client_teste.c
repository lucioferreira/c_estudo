/*
    C ECHO client example using sockets
    gcc -o client_teste client_teste.c
*/
#include<stdio.h> /* printf */
#include<stdlib.h>
#include <unistd.h>
#include<string.h>     /* strlen */
#include<sys/socket.h> /* socket */
#include<arpa/inet.h>  /* inet_addr */

int main()
{
    int sock;
    struct sockaddr_in server;
    char server_reply[2000];
    int opcao = -1;
    char json[100];
    
    memset(&json, 0, sizeof(json) );

    /* Create socket */
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        printf("erro ao criar o socket");
    }
    puts("Socket criado");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );

    /* Connect to remote server */
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("falha ao conectar. Erro");
        return 1;
    }

    puts("Connected\n");
    
    printf("\n\n\n");
 	 printf("0 - sair\n");
 	 printf("1 - autenticar usuario existente\n");
 	 printf("2 - autenticar usuario não existente\n");
 	 printf("3 - autenticar usuario com senha errada\n");
 	 printf("4 - listar_mesa com usuario existente\n");
 	 printf("5 - listar_mesa com usuario não existente\n");
 	 printf("6 - listar_categoria com usuario existente\n");
 	 printf("7 - listar_cardapio com usuario existente\n");
 	 printf("\n\n\n");
    	
    printf("entre com uma opção: ");
	 scanf("%d", &opcao);
	 switch(opcao) {
		case 0:
			close(sock);
			return 0;			
		case 1: /* autenticar usuario existente*/
			puts("autenticar_usuario");
  			strcpy(json, "{\"cmd\":\"autenticar_usuario\", \"usuario\":\"lucio\", \"senha\":\"lucio\"}");
			break;
		case 2: /* autenticar usuario inexistente*/
			puts("autenticar_usuario não existente");
  			strcpy(json, "{\"cmd\":\"autenticar_usuario\", \"usuario\":\"inexistente\", \"senha\":\"lucio\"}");
			break;
		case 3: /* autenticar usuario com senha errada*/
			puts("autenticar_usuario não existente");
  			strcpy(json, "{\"cmd\":\"autenticar_usuario\", \"usuario\":\"lucio\", \"senha\":\"errada\"}");
			break;
		case 4: /* listar_mesas */
			puts("listar_mesa usuario existente");
			strcpy(json, "{\"id_usuario\":\"1\", \"cmd\":\"listar_mesa\"}");
			break;
		case 5: /* listar_mesas */
			puts("listar_mesa usuario não existente");
			strcpy(json, "{\"id_usuario\":\"300\", \"cmd\":\"listar_mesa\"}");
			break;
		case 6: /* listar_categoria */
			puts("listar_categoria usuario existente");
			strcpy(json, "{\"id_usuario\":\"1\", \"cmd\":\"listar_categoria\"}");
			break;
		case 7: /* listar_cardapio */
			puts("listar_cardapio usuario existente");
			strcpy(json, "{\"id_usuario\":\"1\", \"cmd\":\"listar_cardapio\"}");
			break;
			
		default:
			puts("comando inexistente");
			close(sock);
			return 1;
		}    	

  	 memset(&server_reply, 0, sizeof(server_reply) );
  	 /* char *message = "{\"comando\":\"get_mesas\"}"; */
    printf("Enviando json: %s\n", json);

    /* envia o json */
    if( send(sock , json , strlen(json) , 0) < 0) {
       puts("falha no envio");
       return 1;
    }

    /* Receive a reply from the server */
    if( recv(sock , server_reply , 2000 , 0) < 0) {
       puts("recv falhou");
    }

    puts("resposta do Server: ");
    puts(server_reply);
    puts("---  fim  ---");

    close(sock);
 
    return 0;
}

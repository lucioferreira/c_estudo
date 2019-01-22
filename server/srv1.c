/*
    C socket server com json e sqlite3 integrado
    aceita mais de um client
    loop eterno
    cria um processo separado com fork para cada conexão

    gcc -o srv1 srv1.c frozen.c sqlite3.c -lpthread -ldl

    TODO: integração com json e sqlite3
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* strlen */
#include <sys/socket.h>
#include <arpa/inet.h>  /* inet_addr */
#include <unistd.h>     /* write */
#include "frozen.h"     /* json decoder */
#include "sqlite3.h"

#define MAX_JSON 5120
#define SQLITE_DB "servire.db"

void connection_proxy(int); /* função de entrada para cada conexão */
int get_comando(char *json, char *comando); /* retorna o campo "comando" do json */

/* funções API */
int cmd_autenticar_usuario(char *json, char *resposta);
int cmd_listar_mesa(char *json, char *resposta);

/* funções helper */
int get_id_usuario(int id);


int main() {
    int socket_desc , client_sock , c , pid;
    struct sockaddr_in server , client;
    int port = 8888;

    /* cria o socket */
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("Erro ao criar o socket\n");
    }
    puts("Socket criado com sucesso");

    /* prepara o sockaddr_na structure */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );

    /* Bind */ 
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind falhou. Erro");
        return 1;
    }
    printf("bind ok na porta %d\n", port);

    /* Listen */ 
    listen(socket_desc , 3);

    /* Aceita as conexões que estão entrando  */
    puts("Esperando por novas conexões...");
    c = sizeof(struct sockaddr_in);

    /* loop para receber todos as conexões que entram */
    while(1) {

	    /* aceita a conexão de um client */
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
void connection_proxy (int sock) {
   int read_size, ret;
   char buffer[MAX_JSON]; /* armazena o json recebido */
   char resposta[MAX_JSON]; /* armazena a resposta do comando */
   char comando[50];
   memset( &buffer, 0, sizeof(buffer));
   memset( &resposta, 0, sizeof(resposta));
   memset( &comando, 0, sizeof(comando));

	/* Recebe a mensagem do client */
	while( (read_size = read(sock , buffer, sizeof(buffer))) > 0 ) {

		get_comando(buffer, comando);

		/* direciona para a execução dos comandos */ 
	  	if(strcmp(comando, "autenticar_usuario" ) == 0) {
	  		puts("==>autenticar_usuario");
	  		cmd_autenticar_usuario(buffer, resposta);
	  		printf("<== %s\n", resposta);
	  	} else if(strcmp(comando, "listar_mesa" ) == 0) {
	  		puts("==> listar_mesa");
	  		cmd_listar_mesa(buffer, resposta);
	  		printf("<==: %s\n", resposta);
	  	} else {
	  		puts(" ==> comando inexistente <==");
	  	}

         /* Envia a mensagem de volta ao client */
			/*         
         write(sock , buffer , strlen(buffer));
         printf("mensagem: %s\n", buffer);
         */

         /* Envia a mensagem de volta ao client */
         printf("resp2: %s\n", resposta);
         if((ret = write(sock , resposta , strlen(resposta))) == -1) {
            printf("erro no envio para cliente\n");
         } else {
            printf("%d bytes enviados ao cliente\n", ret);
         }

         memset( &resposta, 0, sizeof(resposta));
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

/******** get_comando() *********************
 Retorna o conteúdo do campo "comando"
 enviado dentro do objeto json.
 *********************************************/
int get_comando(char *json, char *comando) {
	char *cmd = NULL;
	/* printf("comando recebido: %s\n", json); */ 
	json_scanf(json, strlen(json), "{cmd:%Q}", &cmd);
	/* printf("comando encontrado: %s\n", cmd); */ 
	strcpy(comando, cmd);
	return 0;
}

/******** cmd_autenticar_usuario() ***************
 Executa o comando da API cmd_autenticar_usuario

 *********************************************/
int cmd_autenticar_usuario(char *json, char *resposta){

	/* pega os campos usuario e senha do json */
	char *usuario;
	char *senha;
	json_scanf(json, strlen(json), "{usuario:%Q, senha:%Q}", &usuario, &senha);

	/* busca no banco de dados */
	sqlite3 *conn;
	sqlite3_stmt *res;
	const char* db = SQLITE_DB;
	int error = 0;

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		return 1;
	}

	char *sql = "select rowid from usuario where usu_nome = ? and usu_senha = ?";

	error = sqlite3_prepare_v2(conn, sql, -1, &res, 0);
	if (error != SQLITE_OK) {
		printf("falha ao buscar dados!: %s\n", sqlite3_errmsg(conn));
		return 1;
	}

	sqlite3_bind_text(res, 1, usuario, -1, 0); /* nome usuario */
	sqlite3_bind_text(res, 2, senha, -1, 0); /* senha */

	/* executa a query */
	int step = sqlite3_step(res);
	if(step == SQLITE_ROW){
		int id = sqlite3_column_int(res,0);
		printf("id_usuario: %d\n", id);
		{
			char buf[2048];
			memset( &buf, 0, sizeof(buf));
			struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
			json_printf(&out, "{status: %Q , resposta: {id_usuario: %d}}", "ok autenticar_usuario", id);
			strcpy(resposta, buf);
		}
	} else if(step == SQLITE_DONE) {
		/* puts("usuario ou senha não encontrado!!"); */ 
		{
			char buf[2048];
			memset( &buf, 0, sizeof(buf));
			struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
			json_printf(&out, "{status: %Q , resposta:%Q}", "erro autenticar_usuario", "usuário inexistente ou senha errada");
			strcpy(resposta, buf);
		}
	}

	sqlite3_finalize(res);
	sqlite3_close(conn);

	return 0;
}

/******** cmd_listar_mesa() ***************
 Executa o comando da API cmd_listar_mesa
 1. verifica se o usuario existe. em caso negativo, retorna erro
 2. retorna a lista de mesas

 *********************************************/
int cmd_listar_mesa(char *json, char *resposta){
	
	char lista_buff[800];
	memset(&lista_buff, 0, sizeof(lista_buff));

	/* pega o campo id do usuario */
	int id_usuario;
	json_scanf(json, strlen(json), "{id_usuario:%d}", &id_usuario);
	
	/* usuario existe? */
	if(get_id_usuario(id_usuario) == -1) {
		char buf[2048];
		memset( &buf, 0, sizeof(buf));
		struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
		json_printf(&out, "{status: %Q , resposta:%Q}", "erro listar_mesa", "id usuario inexistente");
		strcpy(resposta, buf);
		return -1; 
	}

	/* busca no banco de dados */
	sqlite3 *conn;
	sqlite3_stmt *res;
	const char* db = SQLITE_DB;
	int error = 0;

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		sqlite3_close(conn);
		return -1;
	}

	char *sql = "select rowid, titulo, status from mesa order by titulo";

	error = sqlite3_prepare_v2(conn, sql, -1, &res, 0);
	if (error != SQLITE_OK) {
		printf("falha ao buscar dados!: %s\n", sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	}

	/* executa a query */
	/* int step = sqlite3_step(res); */ 
	char lin_buff[150];
	memset(&lin_buff, 0, sizeof(lin_buff));

	/* nenhum registro encontrado */	
	if(sqlite3_step(res) == SQLITE_DONE) {
		/* formata o json de retorno */
		char buf[800];
		memset( &buf, 0, sizeof(buf));
		sprintf(buf, "{\"status\":\"ok listar_mesa\",\"resposta\":[]}");	
	
		strcpy(resposta, buf);
	
		sqlite3_finalize(res);
		sqlite3_close(conn);

		return 0;
	}
	
	strcat(lista_buff, "[");
	while (sqlite3_step(res) == SQLITE_ROW) {
		sprintf(lin_buff, "{\"id_mesa\":%d, \"titulo_mesa\":\"%s\", \"status\":%d},", 
			sqlite3_column_int(res, 0), sqlite3_column_text(res, 1), sqlite3_column_int(res, 2));
		printf("rec: %s\n", lin_buff);
		strcat(lista_buff, lin_buff);
		memset(&lin_buff, 0, sizeof(lin_buff));
	}
	lista_buff[strlen(lista_buff) - 1] = ']';
	printf("lista: %s\n", lista_buff);
	
	/* formata o json de retorno */
	char buf[800];
	memset( &buf, 0, sizeof(buf));
	sprintf(buf, "{\"status\":\"ok listar_mesa\",\"resposta\":%s}", lista_buff);	
	
	strcpy(resposta, buf);
	
	sqlite3_finalize(res);
	sqlite3_close(conn);

	return 0;
}

/******** get_id_usuario() ***************
 Retorna id do usuário
 em caso de usuário desconhecido, retorna -1
 *********************************************/
int get_id_usuario(int id) {
	/* busca no banco de dados */
	sqlite3 *conn;
	sqlite3_stmt *res;
	const char* db = SQLITE_DB;
	int error = 0;
	int ret_id = 0;
	
	printf("==> get_id_usuario id : %d\n", id);

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		sqlite3_close(conn);
		return -1;
	}

	char *sql = "select rowid from usuario where rowid = ?";

	error = sqlite3_prepare_v2(conn, sql, -1, &res, 0);
	if (error != SQLITE_OK) {
		printf("falha ao buscar dados!: %s\n", sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	}

	sqlite3_bind_int(res, 1, id); /* id usuario */

	/* executa a query */
	int step = sqlite3_step(res);
	if(step == SQLITE_ROW){
		ret_id = sqlite3_column_int(res,0);
	} else if(step == SQLITE_DONE) {
		ret_id = -1;
	}

	sqlite3_finalize(res);
	sqlite3_close(conn);
	
	return ret_id;
}
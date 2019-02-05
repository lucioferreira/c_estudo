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

#define MESA_STATUS_LIVRE 1
#define MESA_STATUS_EM_ATENDIMENTO 2
#define MESA_STATUS_RESERVADA 3
#define MESA_STATUS_CONJUGADA 4

#define PEDIDO_STATUS_ABERTO 1
#define PEDIDO_STATUS_PAGO 2
#define PEDIDO_STATUS_CANCELADO 3
#define PEDIDO_STATUS_CORTESIA 4


void connection_proxy(int); /* função de entrada para cada conexão */
int get_comando(char *json, char *comando); /* retorna o campo "comando" do json */


/* funções API */
int cmd_autenticar_usuario(char *json, char *resposta);
int cmd_listar_mesa(char *json, char *resposta);
int cmd_listar_categoria(char *json, char *resposta);
int cmd_listar_cardapio(char *json, char *resposta);
int cmd_abrir_pedido(char *json, char *resposta);
int cmd_fechar_pedido(char *json, char *resposta);


/* funções helper */
int get_id_usuario(int id);
int get_mesa_status(int id);
int set_mesa_status(int id_mesa, int status);
int get_mesa_pedido(int id_pedido);
int set_pedido_status(int id_pedido, int status);


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
	  	} else if(strcmp(comando, "listar_categoria" ) == 0) {
	  		puts("==> listar_categoria");
	  		cmd_listar_categoria(buffer, resposta);
	  		printf("<==: %s\n", resposta);
	  	} else if(strcmp(comando, "listar_cardapio" ) == 0) {
	  		puts("==> listar_cardapio");
	  		cmd_listar_cardapio(buffer, resposta);
	  		printf("<==: %s\n", resposta);
	  	} else if(strcmp(comando, "abrir_pedido" ) == 0) {
	  		puts("==> abrir_pedido");
	  		cmd_abrir_pedido(buffer, resposta);
	  		printf("<==: %s\n", resposta);
	  	} else if(strcmp(comando, "fechar_pedido" ) == 0) {
	  		puts("==> fechar_pedido");
	  		cmd_fechar_pedido(buffer, resposta);
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
	char lin_buff[150];
	memset(&lin_buff, 0, sizeof(lin_buff));

	int step = sqlite3_step(res);

	if(step == SQLITE_DONE) { /* nenhum registro encontrado */
		/* formata o json de retorno */
		char buf[800];
		memset( &buf, 0, sizeof(buf));
		sprintf(buf, "{\"status\":\"ok listar_mesa\",\"resposta\":[]}");

		strcpy(resposta, buf);

		sqlite3_finalize(res);
		sqlite3_close(conn);

		return 0;

	} else if(step == SQLITE_ROW) {
		/* pega o primeiro registro */
		strcat(lista_buff, "[");
		sprintf(lin_buff, "{\"id_mesa\":%d, \"titulo_mesa\":\"%s\", \"status\":%d},",
			sqlite3_column_int(res, 0), sqlite3_column_text(res, 1), sqlite3_column_int(res, 2));
		strcat(lista_buff, lin_buff);
		memset(&lin_buff, 0, sizeof(lin_buff));

		/* pega o demais registros */
		while (sqlite3_step(res) == SQLITE_ROW) {
			sprintf(lin_buff, "{\"id_mesa\":%d, \"titulo_mesa\":\"%s\", \"status\":%d},",
				sqlite3_column_int(res, 0), sqlite3_column_text(res, 1), sqlite3_column_int(res, 2));
			strcat(lista_buff, lin_buff);
			memset(&lin_buff, 0, sizeof(lin_buff));
		}

		lista_buff[strlen(lista_buff) - 1] = ']';
	}

	/* formata o json de retorno */
	char buf[800];
	memset( &buf, 0, sizeof(buf));
	sprintf(buf, "{\"status\":\"ok listar_mesa\",\"resposta\":%s}", lista_buff);

	strcpy(resposta, buf);

	sqlite3_finalize(res);
	sqlite3_close(conn);

	return 0;
}

/******** cmd_listar_categoria() ***************
 Executa o comando da API cmd_listar_categoria
 1. verifica se o usuario existe. em caso negativo, retorna erro
 2. retorna a lista de categorias

 *********************************************/
int cmd_listar_categoria(char *json, char *resposta){

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
		json_printf(&out, "{status: %Q , resposta:%Q}", "erro listar_categoria", "id usuario inexistente");
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

	char *sql = "select rowid, titulo from categoria order by titulo";

	error = sqlite3_prepare_v2(conn, sql, -1, &res, 0);
	if (error != SQLITE_OK) {
		printf("falha ao buscar dados!: %s\n", sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	}

	/* executa a query */
	char lin_buff[150];
	memset(&lin_buff, 0, sizeof(lin_buff));

	int step = sqlite3_step(res);

	if(step == SQLITE_DONE) { /* nenhum registro encontrado */
		/* formata o json de retorno */
		char buf[800];
		memset( &buf, 0, sizeof(buf));
		sprintf(buf, "{\"status\":\"ok listar_categoria\",\"resposta\":[]}");

		strcpy(resposta, buf);

		sqlite3_finalize(res);
		sqlite3_close(conn);

		return 0;

	} else if(step == SQLITE_ROW) {
		/* pega o primeiro registro */
		strcat(lista_buff, "[");
		sprintf(lin_buff, "{\"id\":%d, \"titulo\":\"%s\"},",
			sqlite3_column_int(res, 0), sqlite3_column_text(res, 1));
		strcat(lista_buff, lin_buff);
		memset(&lin_buff, 0, sizeof(lin_buff));

		/* pega o demais registros */
		while (sqlite3_step(res) == SQLITE_ROW) {
			sprintf(lin_buff, "{\"id\":%d, \"titulo\":\"%s\"},",
				sqlite3_column_int(res, 0), sqlite3_column_text(res, 1));
			strcat(lista_buff, lin_buff);
			memset(&lin_buff, 0, sizeof(lin_buff));
		}

		lista_buff[strlen(lista_buff) - 1] = ']';
	}

	/* formata o json de retorno */
	char buf[800];
	memset( &buf, 0, sizeof(buf));
	sprintf(buf, "{\"status\":\"ok listar_categoria\",\"resposta\":%s}", lista_buff);

	strcpy(resposta, buf);

	sqlite3_close(conn);
	sqlite3_finalize(res);

	return 0;
}

/******** cmd_listar_cardapio() ***************
 Executa o comando da API cmd_listar_cardapio
 1. verifica se o usuario existe. em caso negativo, retorna erro
 2. retorna a lista de itens do cardapio

 TODO:
  - analisar o tamanho da resposta
  - campos retornando null devem retornar em branco

 *********************************************/
int cmd_listar_cardapio(char *json, char *resposta){

	char lista_buff[2048];
	memset(&lista_buff, 0, sizeof(lista_buff));

	/* pega o campo id do usuario */
	int id_usuario;
	json_scanf(json, strlen(json), "{id_usuario:%d}", &id_usuario);

	/* usuario existe? */
	if(get_id_usuario(id_usuario) == -1) {
		char buf[2048];
		memset( &buf, 0, sizeof(buf));
		struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
		json_printf(&out, "{status: %Q , resposta:%Q}", "erro listar_cardapio", "id usuario inexistente");
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

	char *sql = "select rowid, cat_cid, titulo, descr_breve, "
						"valor, ext_img "
						"from cardapio order by cat_cid";

	error = sqlite3_prepare_v2(conn, sql, -1, &res, 0);
	if (error != SQLITE_OK) {
		printf("falha ao buscar dados!: %s\n", sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	}

	/* executa a query */
	char lin_buff[450];
	memset(&lin_buff, 0, sizeof(lin_buff));

	int step = sqlite3_step(res);

	if(step == SQLITE_DONE) { /* nenhum registro encontrado */
		/* formata o json de retorno */
		char buf[800];
		memset( &buf, 0, sizeof(buf));
		sprintf(buf, "{\"status\":\"ok listar_cardapio\",\"resposta\":[]}");

		strcpy(resposta, buf);

		sqlite3_finalize(res);
		sqlite3_close(conn);

		return 0;

	} else if(step == SQLITE_ROW) {
		/* pega o primeiro registro */
		strcat(lista_buff, "[");
		sprintf(lin_buff,
		   "{\"id\":%d, \"cat_id\":%d, \"titulo\":\"%s\", \"descr_breve\":\"%s\", \"valor\":%d, \"ext-img\":\"%s\"},",
			sqlite3_column_int(res, 0),
			sqlite3_column_int(res, 1),
			sqlite3_column_text(res, 2),
			sqlite3_column_text(res, 3),
			sqlite3_column_int(res, 4),
			sqlite3_column_text(res, 5));
		strcat(lista_buff, lin_buff);
		memset(&lin_buff, 0, sizeof(lin_buff));

		/* pega o demais registros */
		while (sqlite3_step(res) == SQLITE_ROW) {
		sprintf(lin_buff,
		   "{\"id\":%d, \"cat_id\":%d, \"titulo\":\"%s\", \"descr_breve\":\"%s\", \"valor\":%d, \"ext-img\":\"%s\"},",
			sqlite3_column_int(res, 0),
			sqlite3_column_int(res, 1),
			sqlite3_column_text(res, 2),
			sqlite3_column_text(res, 3),
			sqlite3_column_int(res, 4),
			sqlite3_column_text(res, 5));
		strcat(lista_buff, lin_buff);
			strcat(lista_buff, lin_buff);
			memset(&lin_buff, 0, sizeof(lin_buff));
		}

		lista_buff[strlen(lista_buff) - 1] = ']';
	}

	/* formata o json de retorno */
	char buf[800];
	memset( &buf, 0, sizeof(buf));
	sprintf(buf, "{\"status\":\"ok listar_cardapio\",\"resposta\":%s}", lista_buff);

	strcpy(resposta, buf);

	sqlite3_finalize(res);
	sqlite3_close(conn);

	return 0;
}

/******** cmd_abrir_pedido() ***************
 Executa o comando da API cmd_abrir_pedido
 1. verifica se o usuario existe. em caso negativo, retorna erro
 2. verifica se a mesa está válida (se existe, está ocupada). em caso negativo, retorna erro
    não pode abrir mesa já em atendimento
 3. cria uma entrada na tabela de pedidos
 4. cria uma entrada na tabela de pagamentos (???)
 5. modifica status da mesa para ocupada

 TODO:
  - analisar o tamanho da resposta
  - campos retornando null devem retornar em branco

 *********************************************/
int cmd_abrir_pedido(char *json, char *resposta){

	char buf[2048];
	memset(&buf, 0, sizeof(buf));

	/* pega o campo id do usuario */
	int id_usuario, id_mesa;
	json_scanf(json, strlen(json), "{id_usuario:%d, id_mesa:%d}", &id_usuario, &id_mesa);

	/* usuario existe? */
	if(get_id_usuario(id_usuario) == -1) {
		char buf[2048];
		memset( &buf, 0, sizeof(buf));
		struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
		json_printf(&out, "{status: %Q , resposta:%Q}", "erro abrir_pedido", "id usuario inexistente");
		strcpy(resposta, buf);
		return -1;
	}

	/* retorna status da mesa */
	int mesa_status = get_mesa_status(id_mesa);
	{
		char buf[2048];
		memset( &buf, 0, sizeof(buf));
		switch(mesa_status) {
			case MESA_STATUS_EM_ATENDIMENTO: /* em atendimento  */
            {
				struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
				json_printf(&out, "{status: %Q , resposta:%Q}", "erro abrir_pedido", "mesa em atendimento");
				strcpy(resposta, buf);
				return -1;
				}
			case MESA_STATUS_RESERVADA: /* reservada  */
            {
				struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
				json_printf(&out, "{status: %Q , resposta:%Q}", "erro abrir_pedido", "mesa reservada");
				strcpy(resposta, buf);
				return -1;
				}
			case MESA_STATUS_CONJUGADA: /* conjugada */
            {
				struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
				json_printf(&out, "{status: %Q , resposta:%Q}", "erro abrir_pedido", "mesa conjugada");
				strcpy(resposta, buf);
				return -1;
				}
		}
	}

	/* cria um novo pedido (atendimento) */
	sqlite3 *conn;
	const char* db = SQLITE_DB;
	int error = 0;
	sqlite3_stmt *insert_stmt = NULL;

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		sqlite3_close(conn);
		return -1;
	}

	char *qr_ped = "insert into pedido (mesa_id, usu_id) "
                    " values (?,?)";

    int rc = sqlite3_prepare_v2(conn, qr_ped, -1, &insert_stmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Erro ao preparar o comando de insert no pedido %s (%i): %s\n", qr_ped, rc, sqlite3_errmsg(conn));
		sqlite3_close(conn);
		exit(1);
	}

	sqlite3_bind_int(insert_stmt, 1, id_usuario);
	sqlite3_bind_int(insert_stmt, 2, id_mesa);

	/* executa a inclusão */
	rc = sqlite3_step(insert_stmt);
	if(SQLITE_DONE != rc) {
		fprintf(stderr, "Erro ao inserir pedido no bd (%i): %s\n", rc, sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	} else {
		printf("Pedido inserido com sucesso\n\n");
	}

	/* tenta pegar o id da nova a nova linha inserida */
	sqlite3_int64 novo_id = sqlite3_last_insert_rowid(conn);

	if(set_mesa_status(id_mesa, MESA_STATUS_EM_ATENDIMENTO) != 0) {
        sqlite3_finalize(insert_stmt);
        sqlite3_close(conn);
        return -1;
	}

	sprintf(buf, "{\"status\":\"ok abrir_pedido\",\"resposta\":{\"id_pedido\": %i}}", (int)novo_id );

    strcpy(resposta, buf);

    sqlite3_finalize(insert_stmt);
    sqlite3_close(conn);

	return 0;
}

/******** cmd_fechar_pedido() ***************
 Executa o comando da API cmd_fechar_pedido
 1. verifica se o usuario existe. em caso negativo, retorna erro
 2. modifica status da mesa para livre
 3. modifica status do pedido para fechado

 *********************************************/
int cmd_fechar_pedido(char *json, char *resposta){

char buf[2048];
	memset(&buf, 0, sizeof(buf));

	/* pega o campo id do usuario */
	int id_usuario, id_pedido, id_mesa;
	json_scanf(json, strlen(json), "{id_usuario:%d, id_pedido:%d}", &id_usuario, &id_pedido);

	/* usuario existe? */
	if(get_id_usuario(id_usuario) == -1) {
		char buf[2048];
		memset( &buf, 0, sizeof(buf));
		struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
		json_printf(&out, "{status: %Q , resposta:%Q}", "erro fechar_pedido", "id usuario inexistente");
		strcpy(resposta, buf);
		return -1;
	}

   /* mesa com problemas? */
	id_mesa = get_mesa_pedido(id_pedido);
	if(id_mesa == -1) {
		char buf[2048];
		memset( &buf, 0, sizeof(buf));
		struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
		json_printf(&out, "{status: %Q , resposta:%Q}", "erro fechar_pedido", "id mesa com problema");
		strcpy(resposta, buf);
		return -1;
	}

   /* atualiza o status da mesa */
	if(set_mesa_status(id_mesa, MESA_STATUS_LIVRE) == -1) {
		char buf[2048];
		memset( &buf, 0, sizeof(buf));
		struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
		json_printf(&out, "{status: %Q , resposta:%Q}", "erro fechar_pedido", "status da mesa falhou ao atualizar");
		strcpy(resposta, buf);
		return -1;
	}

	/* atualiza o status do pedido (atendimento) */
	if(set_pedido_status(id_pedido, PEDIDO_STATUS_PAGO) == -1) {
		char buf[2048];
		memset( &buf, 0, sizeof(buf));
		struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
		json_printf(&out, "{status: %Q , resposta:%Q}", "erro fechar_pedido", "status do pedido falhou ao atualizar");
		strcpy(resposta, buf);
		return -1;
	}
	
	sprintf(buf, "{\"status\":\"ok fechar_pedido\",\"resposta\":{\"mensagem\": \"ok\"}}" );

   strcpy(resposta, buf);

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

	/* printf("==> get_id_usuario id : %d\n", id); */

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

/******** get_mesa_status() ***************
 Retorna status da mesa
 em caso de inexistência, retorna -1
 *********************************************/
int get_mesa_status(int id) {
	/* busca no banco de dados */
	sqlite3 *conn;
	sqlite3_stmt *res;
	const char* db = SQLITE_DB;
	int error = 0;
	int ret = 0;

	/* printf("==> get_mesa_status id : %d\n", id); */

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		sqlite3_close(conn);
		return -1;
	}

	char *sql = "select status from mesa where rowid = ?";

	error = sqlite3_prepare_v2(conn, sql, -1, &res, 0);
	if (error != SQLITE_OK) {
		printf("falha ao buscar dados!: %s\n", sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	}

	sqlite3_bind_int(res, 1, id); /* status da mesa */

	/* executa a query */
	int step = sqlite3_step(res);
	if(step == SQLITE_ROW){
		ret = sqlite3_column_int(res,0);
	} else if(step == SQLITE_DONE) {
		ret = -1;
	}

	sqlite3_finalize(res);
	sqlite3_close(conn);

	return ret;
}

/******** set_mesa_status() ***************
 define o status da mesa
 em caso de erro, retorna -1
 *********************************************/
int set_mesa_status(int id_mesa, int status) {

	sqlite3 *conn;
	const char* db = SQLITE_DB;
	int error = 0;
	int ret = 0;
	sqlite3_stmt *update_stmt = NULL;

	/* printf("==> set_mesa_status id_mesa : %d, status: %d\n", id_mesa, status); */

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		sqlite3_close(conn);
		return -1;
	}

	const char *sql = "update mesa set status = ? where rowid = ?";

	error = sqlite3_prepare_v2(conn, sql, -1, &update_stmt, NULL);
	if(SQLITE_OK != error) {
		fprintf(stderr, "Can't prepare insert statment %s (%i): %s\n", sql, error, sqlite3_errmsg(conn));
		sqlite3_close(conn);
		exit(1);
	}

	error = sqlite3_prepare_v2(conn, sql, -1, &update_stmt, NULL);
	if (error != SQLITE_OK) {
		printf("falha ao preparar a query de atualização de  status da mesa!: %s\n", sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	}

	sqlite3_bind_int(update_stmt, 1, status); /* status da mesa */
	sqlite3_bind_int(update_stmt, 2, id_mesa); /* id da mesa - paramentro de busca do rowid */

	/* executa a query */
	int step = sqlite3_step(update_stmt);
	if(step != SQLITE_DONE){
        fprintf(stderr, "erro ao atualizar o status da mesa (%i): %s\n", step, sqlite3_errmsg(conn));
        return -1;
	}

	sqlite3_finalize(update_stmt);
	sqlite3_close(conn);

	return ret;
}


/******** get_mesa_pedido() ***************
 retorna o id da mesa associada a um pedido
 em caso de erro, retorna -1
 *********************************************/
int get_mesa_pedido(int id_pedido){

	sqlite3 *conn;
	sqlite3_stmt *res;
	const char* db = SQLITE_DB;
	int error = 0;
	int ret = 0;

	/* printf("==> get_mesa_pedido id : %d\n", id_pedido); */

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		sqlite3_close(conn);
		return -1;
	}

	char *sql = "select mesa_id from pedido where rowid = ?";

	error = sqlite3_prepare_v2(conn, sql, -1, &res, 0);
	if (error != SQLITE_OK) {
		printf("falha ao buscar dados!: %s\n", sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	}

	sqlite3_bind_int(res, 1, id_pedido);

	/* executa a query */
	int step = sqlite3_step(res);
	if(step == SQLITE_ROW){
		ret = sqlite3_column_int(res,0);
	} else if(step == SQLITE_DONE) {
		ret = -1;
	}

	sqlite3_finalize(res);
	sqlite3_close(conn);

	return ret;

}

/******** set_pedido_status() ***************
 define o status do pedido
 em caso de erro, retorna -1
 *********************************************/
int set_pedido_status(int id_pedido, int status) {
	sqlite3 *conn;
	const char* db = SQLITE_DB;
	int error = 0;
	int ret = 0;
	sqlite3_stmt *update_stmt = NULL;

	printf("==> set_pedido_status id_pedido : %d, status: %d\n", id_pedido, status);

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		sqlite3_close(conn);
		return -1;
	}

	const char *sql = "update pedido set status = ?,  dt_fechamento = datetime(\'now\', \'localtime\') "
							" where rowid = ?";

	error = sqlite3_prepare_v2(conn, sql, -1, &update_stmt, NULL);
	if (error != SQLITE_OK) {
		printf("falha ao preparar a query de atualização de  status do pedido!: %s\n", sqlite3_errmsg(conn));
		sqlite3_close(conn);
		return -1;
	}

	sqlite3_bind_int(update_stmt, 1, status); /* status do pedido mesa */
	sqlite3_bind_int(update_stmt, 2, id_pedido); /* id do pedido - paramentro de busca do rowid */

	/* executa a query */
	int step = sqlite3_step(update_stmt);
	if(step != SQLITE_DONE){
        fprintf(stderr, "erro ao atualizar o status do pedido (%i): %s\n", step, sqlite3_errmsg(conn));
        return -1;
	}

	sqlite3_finalize(update_stmt);
	sqlite3_close(conn);

	return ret;

}
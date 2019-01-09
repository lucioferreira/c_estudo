/*
	gcc -o teste1_sqlite teste1_sqlite.c sqlite3.c -lpthread -ldl
*/

#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

int main(void) {
	sqlite3 *conn;
	sqlite3_stmt *res;
	const char* db = "servire.db";
	int error = 0;
	int rec_count = 0;
	const char *errMSG;
	const char *tail;

	error = sqlite3_open(db, &conn);
	if (error) {
		puts("Falha ao abrir o banco de dados");
		exit(0);
	}

	/*
	error = sqlite3_exec(conn,
		"update  set phonenumber=\'5055559999\' where id=3",
		0, 0, 0);
	*/
	error = sqlite3_prepare_v2(conn,
		"select rowid, usu_nome, usu_senha from usuario order by usu_nome", 1000, &res, &tail);
	if (error != SQLITE_OK) {
		puts("falha ao buscar dados!");
		exit(0);
	}

	puts("==========================");

	while (sqlite3_step(res) == SQLITE_ROW) {
		printf("%d | ", sqlite3_column_int(res, 0));
		printf("%s | ", sqlite3_column_text(res, 1));
		printf("%s\n", sqlite3_column_text(res, 2));

		rec_count++;
	}

	puts("==========================");
	printf("qtde registros lidos: %d.\n", rec_count);

	sqlite3_finalize(res);

	sqlite3_close(conn);

	return 0;
}
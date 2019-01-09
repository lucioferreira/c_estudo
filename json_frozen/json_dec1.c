/*
	https://github.com/cesanta/frozen
*/

#include <stdio.h>
#include <string.h>
#include "frozen.h"

void scan_array(const char *str, int len, void *user_data);
void scan_array_2(const char *str, int len, void *user_data);
void scan_array_ex(const char *str, int len, void *user_data);

int main(){
		
	printf("Frozen Json api teste 1\n\n");
	
	printf("\nEx1 ------------------\n");
	
	const char *j1 = "{\"usu\":\"lucio\", \"pwd\":\"senha lucio\"}";
	
	char *usuario = NULL;
	char *pwd = NULL;
	
	json_scanf(j1, strlen(j1), "{usu:%Q, pwd:%Q}", &usuario, &pwd);
	
	printf("usuario: %s \t\t senha: %s\n", usuario, pwd);


	printf("\nEx2 ------------------\n");
	
	const char *j2 = "{\"status\":\"ok listar_mesas\", \"resposta\":\"[10, 21,32]\"}";
                                      

	char *status = NULL;
	char *resposta = NULL;
	void *dados = NULL;
	
	json_scanf(j2, strlen(j2), "{status:%Q, resposta:[%M]}", &status, &scan_array, dados);
	
	printf("status: %s\n", status);
	
	
	printf("\nEx3 ------------------\n");
	
	const char *j3 = "{\"status\":\"ok listar_mesas\", \
                      \"resposta\":\"[{\"id\":1,\"titulo\":\"Mesa 01\",\"status\":1}, \
                                      {\"id\":2,\"titulo\":\"Mesa 02\",\"status\":1}]\"}";

	char *status2 = NULL;
	char *resposta2 = NULL;
	void *dados2 = NULL;
	
	json_scanf(j3, strlen(j3), "{status:%Q, resposta:[%M]}", &status2, &scan_array_ex, dados2);
	printf("status: %s\n", status);
		
	
	return 0;

}

void scan_array(const char *str, int len, void *user_data) {
    struct json_token t;
    int i;
    printf("Parsing array: %.*s\n", len, str);
    for (i = 0; json_scanf_array_elem(str, len, "", i, &t) > 0; i++) {
      printf("Index %d, token [%.*s]\n", i, t.len, t.ptr);
    }
 }

void scan_array2(const char *str, int len, void *user_data) {
	struct json_token t;
   int i;
   int id, status;
	char *titulo = NULL;

    printf("Parsing array(2): %.*s\n", len, str);
    for (i = 0; json_scanf_array_elem(str, len, "", i, &t) > 0; i++) {
      printf("Index %d, token [%.*s]\n", i, t.len, t.ptr);
      json_scanf(t.ptr, t.len, "{id:%d, titulo:%Q,status:%d}", &id, &titulo, &status);
      printf("[i] id: %d \t titulo:%s \t status: %d\n", id, titulo, status);
    }
 }

 
void scan_array_ex(const char *str, int len, void *user_data) {
   int i;
	void *dados = NULL;
	
	json_scanf(str, strlen(str), "[%M]}", &scan_array2, dados);
 }
 

 
 
 
 
 
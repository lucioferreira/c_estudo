/* 
	servire.db
*/

CREATE TABLE usuario (
  dt_criacao datetime DEFAULT (datetime('now','localtime')),
  usu_tipo integer DEFAULT 1,
  usu_nome varchar(45) DEFAULT NULL,
  usu_senha varchar(40) DEFAULT NULL
);

insert into usuario (usu_nome, usu_senha) values ("lucio", "lucio");
insert into usuario (usu_nome, usu_senha) values ("bruna", "bruna");
insert into usuario (usu_nome, usu_senha) values ("valentina", "vava");
insert into usuario (usu_nome, usu_senha) values ("capitão", "captão");

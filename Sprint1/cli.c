#include <arpa/inet.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>
#include <unistd.h> // close function
#include "../sendTCP.h"//fonction d'envoi du message

//compilation : gcc -o client cli.c ../sendTCP.c
// lancement de cli avec ./client @ip_address_serveur portServeur

int envoieMsg(int dS){
  /*
Fonction qui fait la demande de message dans le terminal
Envoie le message
renvoie si le message est égale à fin
*/
  char msg[256];	
  printf("envoie : ");

  // fgets pour pouvoir écrire plusieurs mots
  fgets(msg, 256, stdin);

  // On remplace le retour charriot par \0
  char *retourCharriot = strchr(msg, '\n');
  *retourCharriot = '\0';

  //scanf("%s", msg);
  //msg[strlen(msg)] = '\0';
  
  int res = sendTCP(dS, msg, strlen(msg) + 1, 0);

  if(res == -1){
    perror("error send Rep");
    exit(0);
  }
  else if(res == 0){
    printf("error send socket closed : %d\n", dS);
    exit(0);
  }

  printf("%d, %s\n", strcmp(msg, "fin"), msg);

  return strcmp(msg, "fin");
}

int afficheMsg(int dS){
  /*
Fonction qui affiche le message reçu uniquement si le message n'est pas 'fin'
Renvoie si le message n'est pas fin
*/
  //reception
  char msgRep[256];
  int recRep = recv(dS, msgRep, sizeof(msgRep), 0);

  if(recRep == -1){
    perror("error recv Rep");
    exit(0);
  }
  else if(recRep == 0){
    printf("error socket recv closed\n");
    exit(0);
  }
  int cmp = strcmp(msgRep, "fin");
  if(cmp != 0) // le msg n'est pas fin
    printf("reçu : %s\n", msgRep);

  printf("%d, %s\n", cmp, msgRep);
  
  return cmp;
}

int main (int argc, char** argv) {

  if(argc != 3){
    printf("Vous devez rentrer: ./client @ip_address_serveur portServeur\n");
    exit(0);
  }

  // Processus père qui crée deux fils, la fille(2) commence par émettre
  // le fils(1) commence par recevoir
  
  int dS;  
  dS = socket(PF_INET, SOCK_STREAM, 0);
	
  if (dS == -1) {
    perror("La socket");
    exit(0);
  }
  printf("create socket success!\n");
  
  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  int inet = inet_pton(AF_INET, argv[1], &(aS.sin_addr));

  if(inet == 0) {
    perror("ip not match");
    exit(0);
  }else if (inet == -1) {
    perror("inet_pton failed");
    exit(0);
  }
  printf("inet success!\n");

  aS.sin_port = htons(atoi(argv[2])); 

  socklen_t lgA = sizeof(struct sockaddr_in);

  //Demande de connexion
  int co = connect(dS, (struct sockaddr *) &aS, lgA);
    
  if(co == -1) {
    perror("connection failed");
    exit(0);
  }
  printf("connection success!\n");

  char msgNum[1];
  int recNum = recv(dS, msgNum, sizeof(msgNum), 0);

  if(recNum == -1){
    perror("error recv connexion");
    exit(0);
  }
  else if(recNum == 0){
    printf("error socket recv connexion closed\n");
    exit(0);
  }
  printf("Client n°%s\n", msgNum);

  int comp = strcmp(msgNum, "1");
  int cmp;
  
  if (comp == 0) {
    // Le numéro du client est 1
    // fille envoie puis reçoit
    while(1){ // tant que le msg n'est pas fin      
      // émission
      cmp = envoieMsg(dS);
      if(cmp == 0)
	break;
      
      //reception
      cmp = afficheMsg(dS);
      if(cmp == 0)
	break;
    }
    close(dS);  
    return 0;
  }
  else {
    // numCli = 2
    // Le fils reçoit en premier
    while(1){
      // reception
      cmp = afficheMsg(dS);
      if(cmp == 0)
	break;

      // émission
      cmp = envoieMsg(dS);
      if(cmp == 0)
	break;
    }
    close(dS);  
    return 0;
  }
}

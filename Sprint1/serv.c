#include <arpa/inet.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>
#include <unistd.h> // close function
#include "../sendTCP.h"//fonction d'envoi du message

int conn(int sockServeur, char* numCli){
  /*
Fonction qui attend une nouvelle connection et créer la socket dans sock
Envoie le numéro du client : 1 ou 2 selon l'ordre de connection
*/
  struct sockaddr_in coord_client;
  int lg = sizeof(coord_client);

  int newSock = accept(sockServeur, (struct sockaddr*)&coord_client, &lg);
  if(newSock == -1){
    printf("error socket creation");
  }
  printf("One client connected! : %d\n", newSock);
  
  int sendCo = sendTCP(newSock, numCli, strlen(numCli), 0);
  
  if(sendCo < 0) {
    printf("Error send connection approval");
  }
  printf("Connection accepted!\n");
  return newSock;
}

int traitement(int sockRecv, int sockSend){
  /*
Fonction qui affiche le message reçu ainsi que sa taille
Envoie le message aux deuxième client
Renvoie la comparaison entre le message et 'fin'
*/
  // recevage
  char msg[256];
  int rec = recv(sockRecv, msg, sizeof(msg), 0);

  if(rec == -1){
    printf("%d to %d : \n", sockRecv, sockSend);
    perror("error recv rec");
    exit(0);
  }
  else if(rec == 0){
    printf("socket %d to socket %d : \n", sockRecv, sockSend);
    printf("error socket recv rec closed\n");
    exit(0);
  }
  else {
        
    printf("reçu (octet): %d\n", rec);
    printf("reçu : %s\n", msg);
  
    // en-voyage
    int res = sendTCP(sockSend, msg, strlen(msg) + 1, 0);

    if(res == -1){
      perror("error recv res");
      exit(0);
    }
    else if(res == 0){
      printf("error socket recv res closed\n");
      exit(0);
    }
  }
  // réinitialise msg
  int cmp = strcmp(msg, "fin");
  memset(msg, 0, strlen(msg));

  return cmp;
}

// lancement de serv.c avec ./server port

int main (int argc, char** argv) {

  if(argc != 2){
    printf("Vous devez rentrer: ./server port\n");
    exit(0);
  }

  /* Déclaration des variables */
  int sockServeur, newSock1, newSock2;
  struct sockaddr_in coord_serv;
    
  coord_serv.sin_family = AF_INET;
  coord_serv.sin_addr.s_addr = INADDR_ANY;
  coord_serv.sin_port = htons(atoi(argv[1]));

  /* Création d'une socket */
  sockServeur = socket(AF_INET, SOCK_STREAM, 0);
  if(sockServeur == -1){
    perror("socket failed");
    exit(0);
  }
  
  /* Serveur : appel BIND */
  if(bind(sockServeur, (struct sockaddr*)&coord_serv, sizeof(coord_serv)) == -1) {
    perror("bind failed");
    exit(0);
  }
    
  /* Serveur : appel LISTEN */
  if(listen(sockServeur, 10) == -1) {
    perror("listen failed");
    exit(0);
  }
  printf("Server listen on port : %s\n", argv[1]);
  
  /* Demande de connection ACCEPT */
  
  while(1){
    /*Demande de connections*/
    int newSock1 = conn(sockServeur, "1");
    int newSock2 = conn(sockServeur, "2");

    /*Communication*/
    int res;
    while(1){
      res = traitement(newSock1, newSock2);
      if(res == 0)
	break;
      
      res = traitement(newSock2, newSock1);
      if(res == 0)
	break;
    }
  }

  close(sockServeur);
  return 0;
}

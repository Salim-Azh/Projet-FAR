#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> // close function
#include "../../sendTCP.h"//fonction d'envoi du message

#define BUFFER_MAX_LENGTH 256


struct sock_struct {
    int cli1;
    int cli2;
};
/*Structure pour accueillir les deux socket et les transmettre au thread*/

void *threadCli1ToCli2(void *args) {
  struct sock_struct *sock = args;

  while(1){
    char msg[256];
    int resR1 = recv(sock -> cli1, msg, sizeof(msg), 0);
    
    if(resR1 == -1){
      perror("Erreur de reception du message du client receveur");
      pthread_exit(NULL);
    }
    else if(resR1 == 0){
      perror("Client1's socket closed");
      pthread_exit(NULL);
    }

    if(strcmp(msg,"fin\n") == 0){  
      sendTCP(sock -> cli2, msg, strlen(msg)+1, 0);
      sendTCP(sock -> cli1, msg, strlen(msg)+1, 0);
      
      close(sock -> cli1);
      close(sock -> cli2);
      printf("Les deux clients sont déconnectés");

      pthread_exit(NULL);
    }
    /*Si le message est "fin" alors on ferme les deux sockets clients*/

    printf("Message recu du Client 1: %s",msg);

    int resSend1 = sendTCP(sock -> cli2, msg, strlen(msg)+1, 0);
    if(resSend1 == -1){
      perror("Erreur transmission du message du client 1");
      pthread_exit(NULL);
    }
    printf("Le message a été transmit à l'autre client");
  }
}

void *threadCli2ToCli1(void *args){
  /*Le client 2 reçoit et le transmet au client 1*/
  struct sock_struct *sock = args;
    
  while(1){
    char msg[BUFFER_MAX_LENGTH];

    int resRecv2 = recv(sock -> cli2, msg, sizeof(msg), 0);
    if(resRecv2 == -1){
      perror("Erreur de reception du message du client 2");
      pthread_exit(NULL);
    }
    else if(resRecv2 == 0){
      perror("Client2's socket closed");
      pthread_exit(NULL);
    }
    /*le serveur recoit le message du client 2*/

    if(strcmp(msg,"fin\n") == 0){
      sendTCP(sock -> cli2, msg, strlen(msg)+1, 0);
      sendTCP(sock -> cli1, msg, strlen(msg)+1, 0);
      /*Si un client envoie fin on envoie fin au deux clients*/

      /*Et on ferme les sockets*/
      close(sock -> cli1);
      close(sock -> cli2);
      printf("Les deux clients sont déconnectés");
      
      pthread_exit(NULL);		
    }

    printf("Message recu du Client 2: %s",msg);
    int resSend2 = sendTCP(sock -> cli1, msg, strlen(msg)+1, 0);
    if(resSend2 == -1){
      perror("Erreur d'envoie du message pour le client 1");
      pthread_exit(NULL);
    }
    printf("Message correctement transmit");
  }	
}

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

// lancement de serv.c avec ./server port

int main (int argc, char** argv) {

  if(argc != 2){
    printf("Vous devez rentrer: ./server port\n");
    exit(0);
  }

  /* Déclaration des variables de socket*/
  int sockServeur, newSock1, newSock2;
  struct sockaddr_in coord_serv;
    
  coord_serv.sin_family = AF_INET;
  coord_serv.sin_addr.s_addr = INADDR_ANY;
  coord_serv.sin_port = htons(atoi(argv[1]));

  /* Création d'une socket */
  sockServeur = socket(PF_INET, SOCK_STREAM, 0);
  if(sockServeur == -1){
    perror("socket failed");
    return EXIT_FAILURE;
  }
  
  /* Serveur : appel BIND */
  if(bind(sockServeur, (struct sockaddr*)&coord_serv, sizeof(coord_serv)) == -1) {
    perror("bind failed");
    return EXIT_FAILURE;
  }
    
  /* Serveur : appel LISTEN */
  if(listen(sockServeur, 10) == -1) {
    perror("listen failed");
    return EXIT_FAILURE;
  }
  printf("Server listen on port : %s\n", argv[1]);

  pthread_t client1;
  pthread_t client2;
  
  /* Demande de connection ACCEPT */
  while(1){
    /*Demande de connections*/
    int newSock1 = conn(sockServeur, "1");
    int newSock2 = conn(sockServeur, "2");

    /*Création des thread pour chacun des clients et redirection vers leurs fonctions respectives*/

    struct sock_struct args;
    args.cli1 = newSock1;
    args.cli2 = newSock2;
    
    if(pthread_create(&client1, NULL, &threadCli1ToCli2, (void*)&args) == -1){
      perror("Error creation thread 1");
      return EXIT_FAILURE;
    }

    args.cli1 = newSock2;
    args.cli2 = newSock1;
    
    if(pthread_create(&client2, NULL, &threadCli2ToCli1, (void*)&args) == -1){
      perror("Error creation thread 2");
      return EXIT_FAILURE;
    }
  }

  int att1 = pthread_join(client1, NULL);
  int att2 = pthread_join(client2, NULL);
  
  close(sockServeur);
  return EXIT_SUCCESS;
}

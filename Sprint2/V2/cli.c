#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h> // close function
#include "../../sendTCP.h"//fonction d'envoi du message

//compilation : gcc -lpthread -o client cli.c ../../sendTCP.c
// lancement de cli avec ./client @ip_address_serveur portServeur

#define BUFFER_MAX_LENGTH 256

/*Variable globale pour pouvoir fermer les thread dans les fonctions*/
pthread_t send_t, recv_t;

void *envoieMsg(int dS){
  /*
    Fonction qui fait la demande de message dans le terminal
    Envoie le message
    renvoie si le message est égale à fin
  */
  while(1) {
    char msg[BUFFER_MAX_LENGTH];  
    // fgets pour pouvoir écrire plusieurs mots
    fgets(msg, BUFFER_MAX_LENGTH, stdin);

    // On remplace le retour charriot par \0
    char *retourCharriot = strchr(msg, '\n');
    *retourCharriot = '\0';
  
    int res = sendTCP(dS, msg, strlen(msg) + 1, 0);

    if(res == -1){
      perror("error send Rep");
      pthread_exit(NULL);
    }
    else if(res == 0){
      printf("error send socket closed : %d\n", dS);
      pthread_exit(NULL);
    }
  }
  pthread_exit(NULL);
}

void *afficheMsg(int dS){
  /*
    Fonction qui affiche le message reçu uniquement si le message n'est pas 'fin'
    Renvoie si le message n'est pas fin
  */
  
  //reception
  char msgRep[BUFFER_MAX_LENGTH];
  while(1) {

    int recRep = recv(dS, msgRep, sizeof(msgRep), 0);

    if(recRep == -1){
      perror("error recv Rep");
      return EXIT_FAILURE;
    }
    else if(recRep == 0){
      printf("error socket recv closed\n");
      return EXIT_FAILURE;
    }
    if(strcmp(msgRep, "fin") == 0) // le msg est fin
      break;
    
    printf("\t%s\n", msgRep);
  }
  
  pthread_cancel(send_t);
  pthread_cancel(recv_t);

  pthread_exit(NULL);
}

int main (int argc, char** argv) {

  if(argc != 3){
    printf("Vous devez rentrer: ./client @ip_address_serveur portServeur\n");
    return EXIT_FAILURE;
  }

  /* Processus père qui crée deux thread un pour l'écoute l'autre pour l'envoie */
  
  int dS = socket(PF_INET, SOCK_STREAM, 0);
  
  if (dS == -1) {
    perror("La socket");
    return EXIT_FAILURE;
  }
  printf("create socket success!\n");
  
  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  int inet = inet_pton(AF_INET, argv[1], &(aS.sin_addr));

  if(inet == 0) {
    perror("ip not match");
    return EXIT_FAILURE;
  }
  else if (inet == -1) {
    perror("inet_pton failed");
    return EXIT_FAILURE;
  }
  printf("inet success!\n");

  aS.sin_port = htons(atoi(argv[2])); 

  socklen_t lgA = sizeof(struct sockaddr_in);

  //Demande de connexion
  int co = connect(dS, (struct sockaddr *) &aS, lgA);
    
  if(co == -1) {
    perror("connection failed");
    return EXIT_FAILURE;
  }
  printf("connection success!\n");
  printf("Entrez votre pseudo : ");

  //Envoie
  if(pthread_create(&send_t, NULL, (void*)envoieMsg, dS) == -1) {
    perror("error thread creation send");
    return EXIT_FAILURE;
  }
      
  //reception
  if(pthread_create(&recv_t, NULL, (void*)afficheMsg, dS) == -1) {
    perror("error thread creation recv");
    return EXIT_FAILURE;
  }

  pthread_join(send_t, NULL);
  pthread_join(recv_t, NULL);

  close(dS);  
  return EXIT_SUCCESS;

}

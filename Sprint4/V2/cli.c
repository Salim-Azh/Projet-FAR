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
#include <dirent.h>
#include <limits.h>
#include <unistd.h> // close function
#include "../../sendTCP.h"//fonction d'envoi du message

//compilation : gcc -lpthread -o client cli.c ../../sendTCP.c
// lancement de cli avec ./client @ip_address_serveur portServeur

#define BUFFER_MAX_LENGTH 256

/*Variable globale pour pouvoir fermer les thread dans les fonctions*/
pthread_t send_t, recv_t;
pthread_t send_file, recv_file;

int createSocketBindConnect(char* ip, int port) {
  int dS = socket(PF_INET, SOCK_STREAM, 0);
  
  if (dS == -1) {
    perror("La socket");
    printf("ip: %s, :%d", ip, port);
    return EXIT_FAILURE;
  }
  printf("create socket success!\n");
  
  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  int inet = inet_pton(AF_INET, ip, &(aS.sin_addr));

  if(inet == 0) {
    perror("ip not match");
    return EXIT_FAILURE;
  }
  else if (inet == -1) {
    perror("inet_pton failed");
    return EXIT_FAILURE;
  }
  printf("inet success!\n");

  aS.sin_port = htons(port); 

  socklen_t lgA = sizeof(struct sockaddr_in);

  //Demande de connexion
  int co = connect(dS, (struct sockaddr *) &aS, lgA);
    
  if(co == -1) {
    perror("connection failed");
    exit(0);
  }
  printf("connection success!\n");

  return dS;
}

void *loopSend(int dS) {
  while(1) {
    envoieMsg(dS);
  }
  pthread_exit(NULL);
}

void envoieMsg(int dS){
  /*
    Fonction qui fait la demande de message dans le terminal
    Envoie le message
    Si le msg est 'file' créée un thread qui demande au serveur de transmettre les structures des clients aux autres afin que le client qui a écrit file devienne un serveur pour l'autre et lui envoie son fichier  
  */
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

void *loopRecv(int dS) {
  while(afficheMsg(dS) != -1) {}
  pthread_cancel(send_t);
  pthread_cancel(recv_t);
  
  pthread_exit(NULL);
}

int afficheMsg(int dS){
  /*
    Fonction qui affiche le message reçu uniquement si le message n'est pas 'fin'
    Si le message reçu est 'file', se prépare à recevoir un fichier d'un autre serveur (le client à qui on paralit avant)
  */
  
  //reception
  char msgRep[BUFFER_MAX_LENGTH];
  int recRep = recv(dS, msgRep, sizeof(msgRep), 0);

  if(recRep == -1){
    perror("error recv Rep");
    exit(0);
  }
  else if(recRep == 0){
    printf("error socket recv closed\n");
    exit(0);
  }

  if(strcmp(msgRep, "fin") == 0) // le msg est fin
    return -1;
    
  else if(strcmp(msgRep, "file") == 0) {
    printf("En attente de réception d'un fichier...\n");

    char nomFichier[100];
				
    int resR = recv(dS, &nomFichier, sizeof(nomFichier), 0);
    printf("%s\n", nomFichier);
      
    char cheminFichier[100] = "../";
    strcat(cheminFichier, nomFichier);

    FILE* monFich = NULL;
    monFich = fopen(cheminFichier, "w+");

    char contenuFichier[100000];
    while (resR = recv(dS, &contenuFichier, sizeof(contenuFichier), 0) <= 0);{ 
      if(fputs(contenuFichier, monFich) < 0)
	perror("probleme creation du fichier");
    }
    fclose(monFich);
    puts("Fichier reçu !");
  }
  else if(strcmp(msgRep, "ok") == 0) {
    return -2;
  }
  else
    printf("\t%s\n", msgRep);

  return 0;
}

int main (int argc, char** argv) {

  if(argc != 3){
    printf("Vous devez rentrer: ./client @ip_address_serveur portServeur\n");
    return EXIT_FAILURE;
  }

  /* Processus père qui crée deux thread un pour l'écoute l'autre pour l'envoie */
  int rcv, sd;  
  
  int dS = createSocketBindConnect(argv[1], atoi(argv[2]));
  printf("Entrez votre pseudo : ");
  envoieMsg(dS);

  /*Ici on reçoit la liste de channel et on envoie le choix*/
  afficheMsg(dS);

  // envoie du nom du salon
  while(afficheMsg(dS) != -2) {
    envoieMsg(dS);
  }
  printf("Vous êtes connectés au salon choisi\n");
  
  //Envoie
  if(pthread_create(&send_t, NULL, (void*)loopSend, dS) == -1) {
    perror("error thread creation send");
    return EXIT_FAILURE;
  }
      
  //reception
  if(pthread_create(&recv_t, NULL, (void*)loopRecv, dS) == -1) {
    perror("error thread creation recv");
    return EXIT_FAILURE;
  }
  
  pthread_join(recv_t, NULL);
  pthread_join(send_t, NULL);

  close(dS);  
  return EXIT_SUCCESS;
}

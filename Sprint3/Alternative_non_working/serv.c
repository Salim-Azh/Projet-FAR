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
#include "../sendTCP.h"//fonction d'envoi du message

// lancement de serv.c avec ./server port NombreConnexion

#define BUFFER_MAX_LENGTH 256
#define MAX_CONNEXION 50
#define PSEUDO_MAX_LENGTH 25

/*structure d'une socket client*/
struct sock_struct {  
  int scli;
  struct sockaddr_in acli;
  pthread_t thread;
  char pseudo[PSEUDO_MAX_LENGTH];
};

/*tableau sock_struct de taille 50*/
struct sock_struct tabSocket[MAX_CONNEXION];
pthread_t thread_send_file;

/*compteur de connections*/
int cptSocket = 0;

void properSend(int i, char* msg) {
  int resSend = sendTCP(tabSocket[i].scli, msg, strlen(msg)+1, 0);
      
  if(resSend == -1){
    pthread_exit(NULL);
  }
}

void* sendStruct(int sender) {
  // on envoie d'abord 'file' à tous pour que les clients se prépare
  broadcast(-2, sender, "file");

  // envoie de la structure
  // sender devient serveur donc on envoie les coordonée du serveur aux autres clients pour qu'il se connecte
  createStruct(sender);
}

void createStruct(int sender) {
  /*Fonction qui envoie la structure du client sender au client recv*/
  // Besoin d'une focntion qui serialize les données de la structure
  // envoie de l'addresse et du port pour pouvoir laisser les clients se connecter en P2P

  // sous la forme : '@ip:port: n°port'

  char* fullMsg;
  int lgIp = sizeof(inet_ntoa(tabSocket[sender].acli.sin_addr));
  int lgPort = sizeof(ntohs(tabSocket[sender].acli.sin_port));
  
  size_t fullSize = lgIp + 1 + lgPort + 1;
  fullMsg = (char*) malloc(fullSize);
	
  strcpy(fullMsg, inet_ntoa(tabSocket[sender].acli.sin_addr));
  strcat(fullMsg, ":");
  strcat(fullMsg, "8081");

  // il faut envoyer au moins le port aux clients meme à celui qui servira de serveur, sauf que le port qu'on lit sert déjà à la communication avec le serveur actuel donc on prendra un port par défault : 8081

  for(int i = 0; i < cptSocket; i++)
    properSend(i, fullMsg);
}

void broadcast(int cmp, int sender, char* msg) {
  /*
Fonction controller de broadcast
Plusieurs cas : cmp == -1; -2; 0
si autre cmp est égal 0 le cas normal d'envoie de msg
*/
  if(cmp == -1) { // on veut envoyer fin à tous le monde
    for(int i = 0; i < cptSocket; i++)
      properSend(i, msg);
  }
  else if(cmp == -2){// on veut envoyer fileSend à l'envoyeur et fileRecv aux autres
    for(int i = 0; i < cptSocket; i++){
      if(i == sender)
	properSend(i, "fileSend");
      else
	properSend(i, "fileRecv");
    }
  }
  else {// on veut envoyer un msg normal donc à tous le monde sauf à l'envoyeur
    for(int i = 0; i < cptSocket; i++) {
      if(i != sender) {
	properSend(i, msg);
      }
    }
  }
}

void createPseudo(int sender, char* msg) {
  /*Créer la chaine 'pseudo: msg'*/
  
  char* fullMsg;
  size_t fullSize = strlen(tabSocket[sender].pseudo) + 1 + strlen(msg) + 1;
  fullMsg = (char*) malloc(fullSize);
	
  strcpy(fullMsg, tabSocket[sender].pseudo);
  strcat(fullMsg, ": ");
  strcat(fullMsg, msg);
 
  broadcast(0, sender, fullMsg);
}

void *controller(int sender) {
  /*Fonction d'envoie à tous les clients sauf celui qui à envoyé
sender étant la position de celui qui a envoyé dans le tableau de socket
*/
  while(1){
    char msg[BUFFER_MAX_LENGTH];
    int res = recv(tabSocket[sender].scli, msg, sizeof(msg), 0);
    
    if(res == -1){
      perror("Erreur de reception du message d'un client");
      pthread_exit(NULL);
    }
    else if(res == 0){
      printf("Client%d's : socket closed\n", sender);
      pthread_exit(NULL);
    }

    if(strcmp(msg,"fin") == 0){
      	/*Si fin on envoie le msg à tous et on les déconnectes tous*/
      broadcast(-1, sender, msg);
      pthread_exit(NULL);
    }

    if(strcmp(msg, "file") == 0) {
      // si le msg est file envoie les structure du client à l'autre
      // la structure est stocké dans tabSocket[i].acli
      // créée un thread
      pthread_create(&thread_send_file, NULL, (void*)&sendStruct, sender);      
    }
    else {
      /*Broadcast du message aux autres*/
      createPseudo(sender, msg);
    }
  }
}

int main (int argc, char** argv) {

  if(argc != 3){
    printf("Vous devez rentrer: ./server port NombreMaxConnexion\navec NombreMaxConnexion<=50\n");
    exit(0);
  }

  int nbMaxConnexion;
  nbMaxConnexion = atoi(argv[2]);
  if (nbMaxConnexion > MAX_CONNEXION){
    printf("Vous devez rentrer: ./server port NombreMaxConnexion\navec NombreMaxConnexion <= 50\n");
    exit(0);
  }

  /* Déclaration des variables de socket*/
  int sockServeur;
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
  if(listen(sockServeur, nbMaxConnexion) == -1) {
    perror("listen failed");
    return EXIT_FAILURE;
  }
  printf("Server listen on port : %s\n", argv[1]);
  
  /*Demande de connection ACCEPT et ajout au tableau de socket*/
  int current;
  while(1){

    while(cptSocket <= nbMaxConnexion){

      int lg = sizeof(tabSocket[cptSocket].acli);
      tabSocket[cptSocket].scli = accept(sockServeur,(struct sockaddr *) &(tabSocket[cptSocket].acli),&lg);
    
      current = cptSocket;
      // Le current est la socket que l'on vient de créer donc toujours un cran derrière le compteur de socket
    
      if(tabSocket[cptSocket].scli == -1){
	printf("error socket creation\n");
	continue;
      }
      else {
	printf("One client connected! : %d\n", tabSocket[current].scli);
	printf("L’adresse IP du client est : %s\n", inet_ntoa(tabSocket[current].acli.sin_addr));
	printf("Son numéro de port est : %d\n", (int) ntohs(tabSocket[current].acli.sin_port));

	cptSocket += 1;
    
	/* Attente du pseudo */
	int rc = recv(tabSocket[current].scli, tabSocket[current].pseudo, sizeof(tabSocket[current].pseudo), 0);
    
	printf("Say welcome to %s\n", tabSocket[current].pseudo);
      }
  
      if(pthread_create(&(tabSocket[current].thread), NULL, (void*)&controller, current) == -1){
	perror("Error creation thread");
	return EXIT_FAILURE;
      }

      printf("Thread créée\n");
    }
    pthread_join(tabSocket[0].thread,NULL);
    /*Dans notre cas si un thread d'un client se termine tous les trhead se termine aussi donc on dit au main d'en attendre qu'un seul et peut importe lequel*/
  }
  
  /*Fremeture de toutes les sockets*/
  for(int i = 0; i < cptSocket; i++){
    close(tabSocket[i].scli);
  }
  cptSocket = 0;
  
  close(sockServeur);
  return EXIT_SUCCESS;
}

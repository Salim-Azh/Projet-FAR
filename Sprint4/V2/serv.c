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

// lancement de serv.c avec ./server port NombreConnexion NbSalon

#define BUFFER_MAX_LENGTH 256
#define PSEUDO_MAX_LENGTH 25
#define MAX_SALON 20
#define MAX_CONNEXION 10

/*structure d'une socket client*/
struct sock_struct {  
  int scli; /*nummero socket client*/
  struct sockaddr_in acli;/*socket client*/
  pthread_t thread;/*thread asscocie*/
  char pseudo[PSEUDO_MAX_LENGTH]; /*pseudo du client*/
};

struct salon_struct {
  int numSalon; /*numero du salon*/
  char nom[20]; /*nom du salon*/
  int nbClients; /*compteur du nombre du client present dans le groupe*/
  int nbMaxCli; /*nombre maximum de client*/
  struct sock_struct tabSocket[MAX_CONNEXION]; /*sockets client du salon*/
};

struct index_struct {
  int indexSalon;
  int indexSocket;
};

/*Un tableau de salon est un tableau de tableau de sock_struct*/
struct salon_struct tabSalon[MAX_SALON];
/*struct sock_struct tabCoToSalon[MAX_CONNEXION];*/

int sockServeur;

pthread_t thread_send_file;

/*compteur de connections*/
int cptSocket = 0;

void properSend(int i, char* msg, int indexSalon) {
  int resSend = sendTCP(tabSalon[indexSalon].tabSocket[i].scli, msg, strlen(msg)+1, 0);
  if(resSend == -1){
    pthread_exit(NULL);
  }
  if(resSend == 0){
    pthread_exit(NULL);
  }
}


/*Retoune le numéro du salon. Si il n'existe retourne -1. Si il est complet retourne -2.
Data nom : nom du salon*/
int checkSalon(char nom[]) {
  int i = 0;
  while (i < MAX_SALON && strcmp(tabSalon[i].nom, nom) != 0) {
    i++;
  }
  
  if (i < MAX_SALON && tabSalon[i].nbClients < tabSalon[i].nbMaxCli) {
    return i;
  }
  else if (tabSalon[i].nbClients == tabSalon[i].nbMaxCli){
    return -2;
  }
  else{
    return -1;
  }
}

/*si cmp = -1 envoi aussi le message msg à l'envoyeur sinon envoi aux autres client sauf envoyeur*/
void broadcast(int cmp, int sender, char* msg, int indexSalon) {
  if(cmp == -1) { // on veut envoyer fin à tous le monde
    for(int i = 0; i < tabSalon[indexSalon].nbClients; i++){
      properSend(i, msg, indexSalon);
    }
  }
  else {// on veut envoyer un msg normal donc à tous le monde sauf à l'envoyeur
    for(int i = 0; i < tabSalon[indexSalon].nbClients; i++) {
      if(i != sender) {
	properSend(i, msg, indexSalon);
      }
    }
  }
}

void createPseudo(int indexSalon, int sender, char* msg) {
  /*Créer la chaine 'pseudo: msg'*/
  char* fullMsg;
  size_t fullSize = strlen(tabSalon[indexSalon].tabSocket[sender].pseudo) + 1 + strlen(msg) + 1;
  fullMsg = (char*) malloc(fullSize);
	
  strcpy(fullMsg, tabSalon[indexSalon].tabSocket[sender].pseudo);
  strcat(fullMsg, ": ");
  strcat(fullMsg, msg);
 
  broadcast(0, sender, fullMsg, indexSalon);
}

/*Parametre : indexSalon indice du salon dans tabSalon ; sender indice de la socket client dans tabSocket du salon*/
//void *controller(int indexSalon, int sender) {
  
void *controller(struct index_struct idx) {
  int indexSalon = idx.indexSalon;
  int sender = idx.indexSocket;

  while(1){
    char msg[BUFFER_MAX_LENGTH];

    printf("antent");
    int res = recv(tabSalon[indexSalon].tabSocket[sender].scli, msg, sizeof(msg), 0);

    printf("recu");
    
    if(res == -1){
      perror("Erreur de reception du message d'un client");
      pthread_exit(NULL);
    }
    else if(res == 0){
      printf("Client%d's : socket closed\n", sender);
      pthread_exit(NULL);
    }

    if(strcmp(msg, "fin") == 0){
      /*Si fin on envoie le msg à tous et on les déconnectes tous*/
      broadcast(-1, sender, msg, indexSalon);

      for(int i = 0; i < tabSalon[indexSalon].nbClients; i++) {
        close(tabSalon[indexSalon].tabSocket[i].scli);
      }
      pthread_exit(NULL);
    }
    /*Broadcast du message aux autres*/
    else
      createPseudo(indexSalon,sender, msg);
  }
}

/*connexion client ajout de la socket au salon*/
void *connexion() {

  while(1) {
    struct sock_struct socket;
    int lg = sizeof(socket.acli);
    socket.scli = accept(sockServeur,(struct sockaddr *) &(socket.acli),&lg);
  
    if(socket.scli == -1){
      printf("error socket creation");
    }
    else {
      printf("One client connected! : %d\n", socket.scli);
      cptSocket += 1;
    
      /* Attente du pseudo */
      int rc = recv(socket.scli, socket.pseudo, sizeof(socket.pseudo), 0);
    
      printf("Say welcome to %s\n", socket.pseudo);
    }
    /*Envoie des salon au nouveau client*/
    char message[500] = "";
    strcat(message, "\n  Salons disponibles : \n");
  
    for (int i = 0; i < MAX_SALON; i++) {
      int num = tabSalon[i].numSalon;
      char numSalon[2];
      sprintf(numSalon, "%d", num);
    
      strcat(message, "Salon n°");
      strcat(message, numSalon);
      strcat(message, " ");
    
      strcat(message, tabSalon[i].nom);
      strcat(message, "\n");
    
      int nbC = tabSalon[i].nbClients;
      char nbCli[2];
      sprintf(nbCli, "%d", nbC);
    
      strcat(message, nbCli);
      strcat(message, "/");
    
      int nbM = tabSalon[i].nbMaxCli;
      char nbMax[3];
      sprintf(nbMax, "%d", nbM);
      
      strcat(message, nbMax);
      strcat(message, "\n");
    }
    strcat(message, "Vous pouvez choisir votre salon en rentrant son nom");
    
    int sd = send(socket.scli, message, sizeof(message), 0);
    
    if (sd < 0) {
      printf("error while sending channel list to client %s", socket.pseudo);
    }
    if (sd == 0) {
      printf("socket of %s ended while sending saloon list", socket.pseudo);
    }
    
    printf("%s à reçu les salons\n", socket.pseudo);
    
    // receive name saloon
    char requestedSalon[20];
    int rcv = recv(socket.scli, &requestedSalon, 20, 0);
    
    if (rcv < 0) {
      printf("error for the choice of channel with client %s", socket.pseudo);
    }
    if (rcv == 0) {
      printf("socket of %s ended", socket.pseudo);
    }
    
    int chosenNum = checkSalon(requestedSalon);
    printf("%d", chosenNum);
    char err1Salon[128] = "Veuillez réessayer avec un nom de salon valide";
    char err2Salon[128] = "Veuillez réessayer plus tard le salon est complet";
    
    while (chosenNum < 0) {
      if(chosenNum == -1){
	sd = send(socket.scli, err1Salon, 128, 0);
	if (sd < 0) {
	  printf("error while sending error channel choice to client %s", socket.pseudo);
	}
	if (sd == 0) {
	  printf("socket of %s ended while sending saloon error choice", socket.pseudo);
	}
      }
      else if(chosenNum == -2){
	sd = send(socket.scli, err2Salon, 128, 0);
	if (sd < 0) {
	  printf("error while sending error channel choice to client %s", socket.pseudo);
	}
	if (sd == 0) {
	  printf("socket of %s ended while sending saloon error choice", socket.pseudo);
	}
      }
      
      memset(requestedSalon, 0, 20);    
      rcv = recv(socket.scli, requestedSalon, 20, 0);
      if (rcv < 0) {
	printf("error recv channel with client %s", socket.pseudo);
      }
      if (rcv == 0) {
	printf("socket of %s ended", socket.pseudo);
      }
      chosenNum = checkSalon(requestedSalon);
    }
    
    char msgOk[3] = "ok";
   
    sd = send(socket.scli, msgOk, 3, 0);
    
    if (sd < 0) {
      printf("error while sending ok channel choice to client %s", socket.pseudo);
    }
    if (sd == 0) {
      printf("socket of %s ended while sending saloon ok choice", socket.pseudo);
    }

    printf("%s veut se connecter au salon %s\n", socket.pseudo, requestedSalon);
    
    int current = tabSalon[chosenNum].nbClients;
    
    tabSalon[chosenNum].tabSocket[current] = socket;
    tabSalon[chosenNum].nbClients += 1;
   
    //    controller(chosenNum, current);
    struct index_struct cur;
    cur.indexSalon = chosenNum;
    cur.indexSocket = current;   

    if(pthread_create(&(tabSalon[chosenNum].tabSocket[current].thread), NULL, (void*)&controller, &cur) == -1){
      perror("Error creation thread controller");
      return EXIT_FAILURE;
    }

    pthread_join(tabSalon[chosenNum].tabSocket[current].thread, NULL);
   
    //pthread_exit(NULL);
  }
}

int main (int argc, char** argv) {

  if(argc != 3){
    printf("Vous devez rentrer: ./server port nbSalon\n");
    exit(0);
  }

  for (int i = 0; i < atoi(argv[3]); i++) {
    printf("Entre un nom pour la conv %d : \n",i);
    fgets(tabSalon[i].nom, 20, stdin);
    
    printf("nombre de personne dans le groupe ? %d : \n",i);
    fgets(tabSalon[i].nbMaxCli, 2, stdin);
  }

  /* Déclaration des variables de socket*/
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
  pthread_t threadConnexion;
  
  if(pthread_create(&(threadConnexion), NULL, (void*)&connexion, NULL) == -1){
    perror("Error creation thread");
    return EXIT_FAILURE;
  }
  
  pthread_join(threadConnexion, NULL);

/*Fremeture de toutes les sockets*/
  for(int i = 0; i < 5; i++) {
    for(int j = 0; j < tabSalon[i].nbClients; j++){
      close(tabSalon[i].tabSocket[j].scli);
    }
  }
  
  close(sockServeur);
  return EXIT_SUCCESS;
}

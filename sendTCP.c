#include "sendTCP.h"
#include <string.h> 
#include <arpa/inet.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h>

int sendTCP(int sock, char* msg, int sizeOcT, int option){

	int res = send(sock, msg, sizeOcT, option);

	if (res == -1 || res == 0){
		return res;
	}

	int countEnvoie = res;

	while (countEnvoie < sizeOcT) {

		res = send(sock, msg + res, sizeOcT - countEnvoie, option);
		
		if (res == -1 || res == 0){
			return res;
		}

		countEnvoie +=res;
	}
	return countEnvoie;
}

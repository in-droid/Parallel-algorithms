/*H********************************************************************************
* Ime datoteke: serverLinux.cpp
*
* Opis:
*		Enostaven stre�nik, ki zmore sprejeti le enega klienta naenkrat.
*		Stre�nik sprejme klientove podatke in jih v nespremenjeni obliki po�lje
*		nazaj klientu - odmev.
*
*H*/

//Vklju�imo ustrezna zaglavja
#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<pthread.h>
/*
Definiramo vrata (port) na katerem bo stre�nik poslu�al
in velikost medponilnika za sprejemanje in po�iljanje podatkov
*/
#define PORT 1236
#define BUFFER_SIZE 256
#define MAX_CONNECTIONS 3

int iConnectionCounter = 0;

void *funkcija_niti(void* );
pthread_t nit[MAX_CONNECTIONS];



struct params {
	int myID;
	int clientSock;
	char buff[BUFFER_SIZE];
	int iResult;
};

struct params argumenti[MAX_CONNECTIONS];

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char **argv){

	//Spremenjlivka za preverjane izhodnega statusa funkcij
	int iResult;

	/*
	Ustvarimo nov vti�, ki bo poslu�al
	in sprejemal nove kliente preko TCP/IP protokola
	*/
	int listener=socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		printf("Error creating socket\n");
		return 1;
	}

	//Nastavimo vrata in mre�ni naslov vti�a
	sockaddr_in  listenerConf;
	listenerConf.sin_port=htons(PORT);
	listenerConf.sin_family=AF_INET;
	listenerConf.sin_addr.s_addr=INADDR_ANY;

	//Vti� pove�emo z ustreznimi vrati
	iResult = bind( listener, (sockaddr *)&listenerConf, sizeof(listenerConf));
	if (iResult == -1) {
		printf("Bind failed\n");
		close(listener);
		return 1;
    }

	//Za�nemo poslu�ati
	if ( listen( listener, 5 ) == -1 ) {
		printf( "Listen failed\n");
		close(listener);
		//return 1;
	}

	//Definiramo nov vti� in medpomnilik
	int clientSock;
	//char buff[BUFFER_SIZE];
	
	/*
	V zanki sprejemamo nove povezave
	in jih stre�emo (najve� eno naenkrat)
	*/
	while (1)
	{
		//Sprejmi povezavo in ustvari nov vti�
		clientSock = accept(listener, NULL, NULL);
		if (clientSock == -1) {
			printf("Accept failed\n");
			//close(listener);
			//return 1;
		
		}
		if (iConnectionCounter < MAX_CONNECTIONS) {
			
			int free;
			for (int i = 0; i < MAX_CONNECTIONS; i++) {
				if (argumenti[i].myID == -1) {
					free = i;
					break;
				}

			}
			argumenti[free].myID = free;
			argumenti[free].clientSock = clientSock;
			argumenti[free].iResult = 0;
			pthread_create(&nit[free], NULL, funkcija_niti, (void *) &argumenti[free]);
			pthread_mutex_lock(&mutex1);
			iConnectionCounter ++;
			pthread_mutex_unlock(&mutex1);

		}
		else {
			close(clientSock);
		}

		//Postrezi povezanemu klientu
		

		//close(clientSock);
		printf("----Clients connected:%d\n", iConnectionCounter);
	}

	//Po�istimo vse vti�e
	close(listener);

	pthread_mutex_destroy(&mutex1);
	
	return 0;
}


void* funkcija_niti(void *args) {
	struct params *argumenti = (struct params *) args;
	int myID = argumenti->myID;
	int iResult = argumenti->iResult;
	int clientSock = argumenti->clientSock;
	char *buff = argumenti->buff;

			do{

			//Sprejmi podatke
			iResult = recv(clientSock, buff, BUFFER_SIZE, 0);
			if (iResult > 0) {
				printf("Bytes received: %d\n", iResult);

				//Vrni prejete podatke po�iljatelju
				iResult = send(clientSock, buff, iResult, 0 );
				if (iResult == -1) {
					printf("send failed!\n");
					close(clientSock);
					break;
				}
				printf("Bytes sent: %d\n", iResult);
			}
			else if (iResult == 0)
				printf("Connection closing...\n");
			else{
				printf("recv failed!\n");
				close(clientSock);
				break;
			}

		} while (iResult > 0);

		pthread_mutex_lock(&mutex1);
		iConnectionCounter --;
		close(clientSock);
		argumenti[myID].myID = -1;
		pthread_mutex_unlock(&mutex1);
		pthread_detach(nit[myID]);
		return 0;
}

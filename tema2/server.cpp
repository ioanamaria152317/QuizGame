#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <iostream>
#include "pugixml.hpp"
#include <map>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/select.h>
#include <vector>


/* portul folosit */
#define PORT 2908
#include "pugixml.hpp"
/* codul de eroare returnat de anumite apeluri */
extern int errno;   //spune compilatorului ca variabila e definita in alta
//parte 

typedef struct quizzQuestion
{
    char question[1024];
    char answer[1024];
    int correctAnswer=0;
} quizzQuestion;
typedef struct clientInfo
{
    char username[50]; 
    int score=0;
    int finished=0;
} clientInfo;
typedef struct thData
{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
}thData;


static void *treat(void *); /* functia executata de fiecare thread ce realizeaza
 comunicarea cu clientii */
void start(void *);
void Clasament();
bool clientsFinished();
int nrClienti=0;
clientInfo clienti[100];   //vector de clienti

pthread_mutex_t mlock=PTHREAD_MUTEX_INITIALIZER;  //sect critice; var partajate

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	//pt sursa de la care se primesc date gen client
  int sd;		//descriptorul de socket 
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	
  //thread- unitate de executie in cadrul unui proces, care e o instanta in 
  //executie 

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

  //SOL_SOCKET se refera la nivelul de socket de baza 
  //SO_REUSEADDR se refera la optiunea dorita, pt ca funct asta configureaza
  //niste opt pt socket ; permite reutilizarea adresei chiar daca este inca 
  // in asteptarea inchiderii, gen TIME_WAIT care e o stare pt asigurarea ca 
  //pachetele au fost corect trimise + procesate; e o per de asteptare 
  //(util cand se schimba rpd adresele si porturile
  // de asc )

  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server)); //umple cu zero uri un bloc de memorie;
  //un fel de memset 
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY); //convertire pt 32 biti
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);    //convertire la fm retelei pt 16 biti
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }
    //am nev de legarea asta dinbtre socket si adr si port pt ca doar pe aici
    //sa accept conexiuni si clientii sa stie unde sa trim cererile lor ca
    //sa interact cu server ul 

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
    {
      int client;
      thData * td; //parametru functia executata de thread     
      socklen_t length = sizeof (from);

      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);

      // client= malloc(sizeof(int));
      /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
	     {
	          perror ("[server]Eroare la accept().\n");
	          continue;
	     }
        /* s-a realizat conexiunea, se astepta mesajul */
       
	td=(struct thData*)malloc(sizeof(struct thData));	
	td->idThread=nrClienti++; //acceseaza campul din struct si creste identif 
    //ca sa fie idenif unici pt thread uri 
	td->cl=client;   //client e rez de la accept

	pthread_create(&th[nrClienti], NULL, &treat, td);	      
			//sa stocheze identif thread ului in th[i], sa faca functia treat
            //si sa utilizeze datele din structura td
       
	  }//while      
}	

static void *treat(void * arg) // poatee primi orice tip de data 
{		
		struct thData tdL; 
		tdL= *((struct thData*)arg);	//tdl ia valoarea lui arg 
		printf ("[thread]- %d - Asteptam participanti...\n", tdL.idThread);
		fflush (stdout);		 
		pthread_detach(pthread_self());		//pthread_self obtine identif 
        //thread ului curent ; resursele la detasare se elibereaza automat 
        //dupa terminarea executiei 
		
    start((struct thData*) arg);
		/* am terminat cu acest client, inchidem conexiunea */
		close ((intptr_t)arg);
		return(NULL);	
}

void start(void * arg)
{
  char nume[256];
	struct thData tdL; 
	tdL= *((struct thData*)arg);
  struct clientInfo participant;
  
	if (read (tdL.cl, nume,sizeof(nume))<=0)
			{
			  printf("[Thread %d]\n",tdL.idThread);
			  perror ("Eroare la read() de la client.\n");
			}
      strcpy(participant.username,nume);
       pthread_mutex_lock(&mlock);
	    strcpy(clienti[tdL.idThread].username,participant.username);
       pthread_mutex_unlock(&mlock);
      
	printf ("[Thread %d]Mesajul a fost receptionat...%s\n",tdL.idThread, participant.username);
		      
		      /*pregatim mesajul de raspuns */
		      
  char mesaj[256]="Ai intrat in joc! Quizz-ul va incepe in cateva momente!";   
	//printf("[Thread %d]Trimitem mesajul inapoi...%s\n",tdL.idThread, mesaj);
  
		      /* returnam mesajul clientului */
	 if (write (tdL.cl, mesaj, sizeof(mesaj)) <= 0)
		{
		 printf("[Thread %d] ",tdL.idThread);
		 perror ("[Thread]Eroare la write() catre client.\n");
		}
	 //else
		//printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
    sleep(10);
          /*incarc documentul xml cu intrebari*/
    pugi::xml_document doc;
    if (!doc.load_file ("quizzQuestions.xml"))
    std::cout<< "eroare la incarcarea documentului cu intrebari";

    struct quizzQuestion q1;
    pugi::xml_node root = doc.child("quiz");
    int count=0;

         /*parcurg fisierul pt intrebari*/
    for (pugi::xml_node questionNode= root.child("question"); count < 5; questionNode = questionNode.next_sibling("question"), ++count) //merge prin fiecare nod (adica intrebare)
      {
           q1.correctAnswer=0;
           strcpy(q1.question, questionNode.child_value("text"));
           if ( write(tdL.cl,q1.question,sizeof(q1.question) )<0)
             {
               printf("[Thread %d] ",tdL.idThread);
               perror("Eroare la trimiterea intrebarii!");
             }
           //else 
             //{
		           //printf ("[Thread %d]Intrebarea a fost transmisa cu succes.\n",tdL.idThread);	
            // }
          // printf ("%s\n",q1.question);
           int count1=0;
           
         /*parcurg fisierul pt raspunsuri*/
          for (pugi::xml_node answerNode= questionNode.child("answers").child("answer"); count1 < 4; answerNode = answerNode.next_sibling("answer"), ++count1)
            {
                std::string answerText=answerNode.child_value();
                strcpy(q1.answer,answerText.c_str());
                if (write(tdL.cl,q1.answer,sizeof(q1.answer))<=0)
                   {
                      printf("[Thread %d]", tdL.idThread);
                      perror("Eroare la trimiterea raspunsului!");
                   }
                ///else
                   //{
                  //printf ("[Thread %d] Raspunsul a fost trimis cu succes!\n ", tdL.idThread);
                   //}
            }
           
           fd_set rfds; //descr pt citire
           int retval;
           struct timeval nsecunde;
           FD_ZERO(&rfds);  //macro pt initializare
           FD_SET(tdL.cl, &rfds); //adauga desc fiecarui socket la setul de citire

           char litera;
           nsecunde.tv_sec = 15;
           nsecunde.tv_usec = 0;
           retval = select(tdL.cl+2, &rfds, NULL, NULL, &nsecunde);
//nr de desc
           if (retval == -1)
           {
               perror("select()");
           }
           else if (retval==0)
               {
                  printf("No data within 15 seconds.\n");
                  continue;
               }
           else
               {
               printf("Data is available now.\n");
               if(FD_ISSET(tdL.cl, &rfds))
                 { 
                    size_t bytesRead=read(tdL.cl, &litera,sizeof(char));
                    if (bytesRead<0)
                      {
                         perror("Eroare la primirea raspunsului!");
                      }
                    else if(bytesRead==0)
                      {
                     printf("Client disconnected!\n");
                     pthread_mutex_lock(&mlock);
                     clienti[tdL.idThread].finished=1;
                     pthread_mutex_unlock(&mlock);
                     close(tdL.cl);
                     return;
                     FD_CLR(tdL.cl,&rfds);
                      }
                    else
                      {
                      printf ("litera este: %c\n",litera);
                      }
                 }
              
                }

         /*parcurg fisierul pt verificarea raspunsului*/
          for (pugi::xml_node answerNode : questionNode.child("answers").children("answer"))
           {
             bool isCorrect=answerNode.attribute("correct").as_bool(false); //daca nu exista atr
//sau nu se poate face conversia la bool o sa fie automat setat la false 
             std::string answerText=answerNode.child_value();
             //std::string litera=answerText.substr(0,1);
             char primulCaracter=answerText.at(0);
             if (litera==primulCaracter && isCorrect ) 
                {
                  q1.correctAnswer=1;
                  participant.score++;
                }
           }

        //printf("correct answer este %d\n",q1.correctAnswer);
        if (write(tdL.cl,&q1.correctAnswer,sizeof(q1.correctAnswer))<0)
        {
            perror("Eroare la trimiterea valorii variabilei correct answer!"); 
        }
      }
          
          clienti[tdL.idThread].finished=1;
            
        std::map < std::string , int > punctaje;
        punctaje [participant.username]=participant.score;
        pthread_mutex_lock(&mlock);
        clienti[tdL.idThread].score=participant.score;
        pthread_mutex_unlock(&mlock);
        for (const auto& user : punctaje)
           {
             std::cout<<"Username:  "<<user.first<<std::endl<<"Score:  "<<user.second<<" "<<std::endl;
           }
        
        char anunt_final_quiz[100]="Quiz ul s-a terminat, va fi anuntat castigatorul!";
        printf("%s\n",anunt_final_quiz);
        /*anuntare castigator*/
        if(write(tdL.cl, anunt_final_quiz, sizeof(anunt_final_quiz))<=0)
        {
          perror("Eroare la trimiterea anuntului de final de quiz!");
        }
        if(write(tdL.cl,&participant.score,sizeof(int))<=0)
        {
          perror("Eroare la trimiterea punctajului!");
        }

          while (!clientsFinished())
          {
                continue; //merge in while pana cand se indeplineste conditia
          }
         
          Clasament();
          if (write(tdL.cl,clienti[nrClienti-1].username,sizeof(clienti[nrClienti-1].username))<0)
          {
             perror("Eroare la trimiterea castigatorului!");
          }

}

bool clientsFinished()
{
  for (int i=0;i<nrClienti;i++)
  {
    if(clienti[i].finished==0) 
      {
        return false;
      }
  }
  return true;
}


void Clasament()
{
    for(int i=0;i<nrClienti;i++)
    {
      for(int j=0;j<nrClienti-1;j++)
       {
        if (clienti[j].score>clienti[j+1].score)
        {
        std::swap(clienti[j],clienti[j+1]);
        }
       }
    }
      std::cout<<"Castigatorul este:  "<<clienti[nrClienti-1].username<<"cu scorul:"<<clienti[nrClienti-1].score<<std::endl;
}

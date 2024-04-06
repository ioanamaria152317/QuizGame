#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;
/* portul de conectare la server*/
int port;

typedef struct quizzQuestion
{
    char question[1024];
    char answer[4][1024];
    int correctAnswer=0;
} quizzQuestion;

int main (int argc, char *argv[])
{
     int sd;	
     		// descriptorul de socket
     struct quizzQuestion q;
     struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
     char nume[256];
     char buf[256];
     
  /* exista toate argumentele in linia de comanda? */
     if (argc != 3)
       {
         printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
         return -1;
       }

  /* stabilim portul */
     port = atoi (argv[2]);

  /* cream socketul */
     if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
       {
         perror ("Eroare la socket().\n");
         return errno;
       }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
      if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
        {
         perror ("[client]Eroare la connect().\n");
         return errno;
        }

  /* citirea mesajului */
      printf ("[client]Introduceti un nume: ");
      fflush (stdout);
      read (0, buf, sizeof(buf));
      strcpy(nume,buf);
  
      //printf("[client] Am citit %s\n",nume);

  /* trimiterea mesajului la server */
      if (write (sd,nume,sizeof(nume)) <= 0)
        {
          perror ("[client]Eroare la write() spre server.\n");
          return errno;
        }
      memset(nume,0,256);
  /* citirea raspunsului dat de server 
     (apel blocant pina cind serverul raspunde) */
      if (read (sd, nume,sizeof(nume)) < 0)
        {
          perror ("[client]Eroare la read() de la server.\n");
          return errno;
        }
  /* afisam mesajul primit */
      printf (" %s\n", nume);

    for(int i=0;i<5;i++)
      {  
           memset(q.question,0,1024);
           if (read(sd,q.question , sizeof(q.question))<0)
              {
                 perror("eroare la primirea intrebarii de la server!");
              }
           printf ("%s\n", q.question);

           for (int j=0;j<4;j++)
              {
                 memset(q.answer[j],0,1024); //sa nu fac suprascriere
                 if (read(sd, q.answer[j],sizeof(q.answer[j]))<0)
                     {
                        perror("Eroare la primirea variantei de raspuns de la server!");
                     }
                 printf ("  %s\n", q.answer[j]);
              }
          
           printf ("Write your answer here:");
           fflush (stdout);
           char litera;
           char buff[2];
           memset(buff,0,sizeof(buff));

           fd_set rfds; //descr pt citire
           int retval;
           struct timeval nsecunde;
           FD_ZERO(&rfds);  //macro pt initializare
           FD_SET(0, &rfds); 
           nsecunde.tv_sec = 15;
           nsecunde.tv_usec = 0;
           retval = select(1, &rfds, NULL, NULL, &nsecunde);
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
               //printf("Data is available now.\n");
                   int bytesRead=read (0, buff, sizeof(buff));
                   litera=buff[0];
               if (bytesRead<0)
                   {
                     perror("Eroare la primirea raspunsului!");
                   }
               else if(bytesRead==0)
                   {
                     printf("Client disconnected!\n");
                   }
               else
                   {
                     printf (" %c\n",litera);
      
               
           if (write (sd,&litera,sizeof(char)) <= 0)
               {
                perror ("[client]Eroare la write() spre server.\n");
               }
           
           if (read(sd,&q.correctAnswer,sizeof(int))<0)
               {
                 perror("Eroare la primirea variabilei!");
               }
           if (q.correctAnswer==1)
               {
                 printf("Raspuns corect!\n");
               }
           else 
                 printf ("Try next time!\n");
                   }
               }
      }
      char anunt_final[100];
      if(read(sd,anunt_final,sizeof(anunt_final))<0)
      {
        perror("Eroare la primirea anuntului de la server!");
      }
      printf("%s\n",anunt_final);
      int punctaj_final=0;
      if(read(sd,&punctaj_final,sizeof(int))<0)
      {
        perror("Eroare la primirea punctajului!");
      }
      printf ("Scorul tau este: %d\n", punctaj_final);
      
      char castigator[50];
      if (read(sd,castigator,sizeof(castigator))<0)
      {
        perror("Eroare la primirea numelui castigatorului!");
      }
      printf("Castigatorul este: suspanssssss\n");
      sleep(10);
      printf("%s !!!!!!!!!\n",castigator);
      printf("Felicitari tuturor!\n");
  /* inchidem conexiunea, am terminat */
  close (sd);
}
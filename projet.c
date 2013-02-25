/*
//////////////////////////////////////////////////
projet.c
Alexis Montigny
Traver Maxime
/////////////////////////////////////////////////
*/

#include <stdio.h>
#include <mpi.h>

typedef enum
{
  ATTENTE,
  CALCUL,
  DEFFAILLANT,
  DEFFAILLANT_TRAITE
} Etat;

typedef struct
{
  Etat etat;
  int tranche[2];
  double startTime;
} Process;

int main (int argc, char *argv[]){
  int rank,size;
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  MPI_Status status;

  int N = atoi(argv[1]);
  int tranche = 3;
  if(tranche>N)tranche=N;
  int stop = 0;

  if(rank>0) //SLAVE
  {
    stop = 0;
    while(stop==0)
    {
      int buf[2];
      MPI_Status status;
      MPI_Recv(&buf, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if(status.MPI_TAG == 0)
      {
        int i;
        int sum=0;
        for(i=buf[0];i<=buf[1];i++){sum += i*i;}

        printf("somme de l'esclave %d : %d\n",rank, sum);
        MPI_Send(&sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      }
      else
      {
        printf("esclave %d tué\n",rank);
        stop=1;
      }
    }
  }
  else // MASTER
  {

    Process P[size-1];

    int send[2];
    send[0]=1;
    send[1]=tranche;
    int i;

///////////////       
//phase1///////
///////////////

    // Pour chaque process
    for(i=1;i<size;i++)
    {

      // Si pas/plus de tranche a calculer pour l'instant
      if(send[1] > N)
      {
        P[i-1].etat = ATTENTE;
      }
      // Si tranche a calculer
      else
      {
        // Envoi de la tranche
        MPI_Send(&send,2,MPI_INT,i,0,MPI_COMM_WORLD);
        P[i-1].etat = CALCUL;
        P[i-1].tranche[0] = send[0];
        P[i-1].tranche[1] = send[1];
        P[i-1].startTime = MPI_Wtime();
        
        if(i!=size-1) // Sauf pour le dernier
        {
          // Calcul de la prochaine tranche
          send[0] += tranche;
          send[1] += tranche;
          if(send[1] > N) send[1] = N;
        }
      }
    }



///////////////
//phase2///////
///////////////

    int res,somme;
    MPI_Request request;
    int flag;
    somme=0;
    res=0;
    stop=0;
    while(stop == 0)
    {

      //MPI_Recv(&res,1,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
      MPI_Irecv(&res,1, MPI_INT, MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &request);
      
      MPI_Test(&request, &flag, &status);

      somme += res;
      printf("reception de %d\n",status.MPI_SOURCE);
      P[status.MPI_SOURCE-1].etat = ATTENTE;
                                          
      // Si il reste des tranches a calculer
      if(send[1]<N)
      {
        // Calcul de la prochaine tranche
        send[0] += tranche;
        send[1] += tranche;
        if(send[1] > N) send[1] = N;    
        // Envoi de la prochaine tache au premier process au statut ATTENTE
        int dispo=0;
        for(i=1;i<size;i++)
        {
          if(P[i-1].etat==ATTENTE) 
          {
            dispo=i;
            break;
          }
        }
        // Si un statut est en ATTENTE
        if(dispo!=0)
        {
          MPI_Send(&send,2,MPI_INT,dispo,0,MPI_COMM_WORLD);
          P[dispo-1].etat = CALCUL;
          P[dispo-1].tranche[0] = send[0];
          P[dispo-1].tranche[1] = send[1];
          P[dispo-1].startTime = MPI_Wtime();
        }
        // Sinon on reviens a la tranche precedente vu qu'on n'a pas envoye celle ci pour l'instant
        else
        {
          send[0] -= tranche;
          send[1] -= tranche;
        }
      }
      else
      {
          // Verification de l'etat des process
          int tous_finis=1;
          for(i=0;i<size-1;i++)
          {
              if(P[i].etat==CALCUL) tous_finis=0; //Calculs restants
              if(P[i].etat==DEFFAILLANT) //Process defaillant a traiter
              {
                tous_finis=0;
                  
              }
          }
          // Et si tous les process ont termin�
          if(tous_finis==1)
          {
            // On sort de la boucle
            stop=1;
          }
          else
          {
            // On reaffecte les tranches des deffaillants                                 
          }
      }
    }


///////////////
//phase 3//////
///////////////


    for(i=1;i<size;i++)
    {
      MPI_Send(&send,2,MPI_INT,i,1,MPI_COMM_WORLD);
    }

    printf("\nRESULTAT : %d\n",somme);

  }
  printf("processus %d terminé\n",rank);
  exit(0);
  return 0;
}

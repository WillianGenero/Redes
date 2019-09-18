#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <pthread.h>
 #include <unistd.h>

#define NODES 6
int caminho[NODES][NODES];

#define SERVER "127.0.0.1"
#define BUFLEN 512  //Max length of buffer
#define PORT_0 8888
#define PORT_1 8889

int searchMinor();
void savePath();
void dijkstra();
void carregaEnlaces();

void *sender();
void *receiver();

void die(char *s)
{
    perror(s);
    exit(1);
}


int main(void)
{
    pthread_t t[2];
    int *meuid, i;
    meuid = malloc(sizeof(int));
    *meuid = 1;
    char teste[100];

    printf("Olá, por favor informe o meu ID: ");
    scanf("%d", meuid);

    // todo: aqui vai o reconhecimento do enlaces
    carregaEnlaces();
    
    pthread_create(&t[1], NULL, sender, (void *)meuid);
    pthread_create(&t[2], NULL, receiver, (void *)meuid);
    
    for(i=0; i<2; i++) {
      pthread_join(t[i], NULL);
      printf("Thread id %ld returned\n", t[i]);
   }
   return(1);
}

int searchMinor(int dijkstra[3][NODES]){
    int minor = 9999, id = -1;
    for (int j=0 ; j<NODES ; j++){
        if (dijkstra[2][j] == 0 && dijkstra[0][j] < minor){
            minor = dijkstra[0][j];
            id = j;
        }
    }
    return (minor == 9999) ? -1 : id;
}

void savePath(int dijsktra[3][NODES], int column, int start, int id){
    if (column==id)
        caminho[id][id] = id;
    else if (dijsktra[1][column] == id)
        caminho[id][start] = column;
    else
        savePath(dijsktra, dijsktra[1][column], start, id);
}

void dijkstra(int adjacencia[NODES][NODES]){
    int dijkstra[3][NODES];
    
    for(int i=0; i<NODES ; i++){
        int id = i, custo = 0;

        memset(dijkstra, 0, sizeof(dijkstra));
        memset(dijkstra, -1, 2*NODES*sizeof(int));
        memset(dijkstra, 9999,  NODES*sizeof(int));

        dijkstra[0][id] = 0;
        dijkstra[1][id] = 0;

        while(id != -1){
            for (int j=0 ; j<NODES ; j++){
                dijkstra[2][id] = 1;
                if (dijkstra[2][j] == 0 && adjacencia[id][j] != 0 && (adjacencia[id][j] + custo) < dijkstra[0][j]){
                    dijkstra[0][j] = (adjacencia[id][j] + custo);
                    dijkstra[1][j] = id;
                }
            }
            id = searchMinor(dijkstra);
            custo = dijkstra[0][id];
        }
        for (int k=0 ; k<NODES ; k++)
            savePath(dijkstra, k, k, i);
    }
}

void carregaEnlaces(){
    int adjacencia[NODES][NODES];
    int rot1, rot2, custo;

   memset(adjacencia, 0, sizeof(adjacencia));

    FILE *file = fopen("enlaces.config", "r");
    if (!file)
        printf("Não foi possível abrir o arquivo de Enlaces");

    //Lê os 3 valores e salva na matriz
    while (fscanf(file, "%d %d %d", &rot1, &rot2, &custo) != EOF){
        adjacencia[rot1][rot2] = custo;
        adjacencia[rot2][rot1] = custo;
    }
    fclose(file);
    dijkstra(adjacencia);
}

void *sender() 
{
    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN];    
 
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT_0);
     
    if (inet_aton(SERVER , &si_other.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    while(1)
    {
        printf("Enter message: ");
        //gets(message);
        scanf("%s",message);
         
        //send the message
        if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
        {
            die("sendto()");
        }
         
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            die("recvfrom()");
        }
         
        // puts(buf);
    }

    close(s);
    return 0;
}

void *receiver() 
{
    struct sockaddr_in si_me, si_other;
     
    int s, i, slen = sizeof(si_other) , recv_len;
    char buf[BUFLEN];
     
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT_1);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
     
    //keep listening for data
    while(1)
    {
        printf("Waiting for data...");
        fflush(stdout);
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
         
        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %s\n" , buf);
         
        //now reply the client with the same data
        if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }
    }
 
    close(s);
    return 0;
}
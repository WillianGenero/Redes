#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include<unistd.h>

#define NODES 6

#define SERVER "127.0.0.1"
#define BUFLEN 100
#define PORT_0 8888

int searchMinor();
void savePath();
void dijkstra();
void carregaEnlaces();
void carregaConfigs(int adjacentes[]);

void *sender();
void *receiver(void *porta);

struct roteador
{
    int id;
    int porta;
};

struct roteador *roteadores;
int caminho[NODES][NODES], adjacencia[NODES][NODES];
int n_adj=1;   //n_adj começa com um porque já considera sua própria porta

void die(char *s)
{
    perror(s);
    exit(1);
}

int main(void)
{
    int *meuid, i, j, adjacentes[NODES];
    meuid = malloc(sizeof(int));
    
    memset(&adjacentes, 0, sizeof(int) * NODES);

    printf("Olá, por favor informe o meu ID: ");
    scanf("%d", meuid);

    carregaEnlaces();
    dijkstra();
    
    adjacentes[0] = *meuid;                         //usa o proprio id só pra buscar a porta no
    for(i=0; i<NODES; i++) {
        if(adjacencia[*meuid][i]){
            adjacentes[n_adj] = i;
            n_adj++;
        }
    }

    carregaConfigs(adjacentes);

    pthread_t tids[2];

    puts("Roteador atual e vizinhos:");
    for(int i=0; i<n_adj; i++)
        printf("roteador:%d | porta:%d\n", roteadores[i].id, roteadores[i].porta);

    pthread_create(&tids[0], NULL, receiver, (void *) &roteadores[0].porta);
    pthread_create(&tids[1], NULL, sender, NULL);
    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);

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

void dijkstra(){
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
    int rot1, rot2, custo;

   memset(adjacencia, 0, sizeof(adjacencia));

    FILE *file = fopen("enlaces.config", "r");
    if (!file)
        die("Não foi possível abrir o arquivo de Enlaces");

    //Lê os 3 valores e salva na matriz
    while (fscanf(file, "%d %d %d", &rot1, &rot2, &custo) != EOF){
        adjacencia[rot1][rot2] = custo;
        adjacencia[rot2][rot1] = custo;
    }
    fclose(file);
}

void carregaConfigs(int adjacentes[])
{
    int id_rot, porta_rot;
    FILE *file;
    roteadores = malloc(sizeof(struct roteador) * n_adj);

    for(int i=0; i<n_adj; i++){
        file = fopen("roteador.config", "r");
        if (!file)
            die("Não foi possível abrir o arquivo de Roteadores");

        while (fscanf(file, "%d %d", &id_rot, &porta_rot) != EOF){
            if(adjacentes[i] == id_rot){
                roteadores[i].id    =  id_rot;
                roteadores[i].porta =  porta_rot;
            }
        }
        fclose(file);
    }
}

void *sender() 
{
    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN];
    int id_destino;
 
    while(1)
    {
        if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            die("socket");
        }

        printf("Enter router id:\n");
        scanf("%d", &id_destino);
        printf("Enter message:\n");
        scanf("%s", message);

        for(i=1; i<n_adj; i++){
            if (roteadores[i].id == id_destino)
                break;
        }

        if(i==n_adj){
            puts("o roteador destino não é acessível");
            continue;
        }

        printf("porta destino %d\n", roteadores[i].porta);

        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(roteadores[i].porta);
        
        if (inet_aton(SERVER , &si_other.sin_addr) == 0) 
        {
            fprintf(stderr, "inet_aton() failed\n");
            exit(1);
        }        
         
        if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
        {
            die("sendto()");
        }
        close(s);
    }

    return 0;
}

void *receiver(void *porta) 
{
    struct sockaddr_in si_me, si_other;
     
    int s, i, slen = sizeof(si_other) , recv_len;
    char buf[BUFLEN];
    int port = *((int*) porta);

    printf("Escutando na porta  %d\n", port);
     
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
     
    //keep listening for data
    while(1)
    {
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
    }
 
    close(s);
    return 0;
}
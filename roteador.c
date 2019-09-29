#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include<unistd.h>
#include <stdio_ext.h>
#include "headers/structures.h"

int searchMinor();
void savePath();
void dijkstra();

void loadLinks();
void loadConfs(int adjacentes[]);
void socketConfig();

void sendPacket(pacote packet);

void *terminal();
void *router(void *porta);

struct roteador *roteadores;
struct sockaddr_in si_me, si_other;
pthread_mutex_t timerMutex = PTHREAD_MUTEX_INITIALIZER;
int caminho[NODES][NODES], adjacencia[NODES][NODES];
int n_adj = 1, sock, seq = 0, confirmacao = 0, tentativa = 0;

void die(char *s)
{
    perror(s);
    exit(1);
}

void printRoteadores(){
    puts("---------------------------------");
    printf("| id:%d %s:%d\n| Vizinhos:\n", roteadores[0].id, roteadores[0].ip, roteadores[0].porta);
    for(int i=1; i<n_adj; i++)
        printf("| id:%d | %s:%d\n", roteadores[i].id, roteadores[i].ip, roteadores[i].porta);
    puts("---------------------------------");
}

int main(int argc, char *argv[ ])
{
    int *meuid, i, j, adjacentes[NODES];
    meuid = malloc(sizeof(int));

    if(argc==1){
        printf("Olá, por favor informe o meu ID:\n");
        scanf("%d", meuid);
    }else if(argc==2){
        *meuid = atoi(argv[1]);
        printf("Olá, sou o roteador %d:\n", *meuid);
    }
    
    memset(&adjacentes, 0, sizeof(int) * NODES);
    loadLinks();
    dijkstra();
    
    adjacentes[0] = *meuid;                         //usa o proprio id só pra buscar a porta no
    for(i=0; i<NODES; i++) {
        if(adjacencia[*meuid][i]){
            adjacentes[n_adj] = i;
            n_adj++;
        }
    }

    loadConfs(adjacentes);

    pthread_t tids[2];

    printRoteadores();

    socketConfig();    

    pthread_create(&tids[0], NULL, router, (void *) &roteadores[0].porta);
    pthread_create(&tids[1], NULL, terminal, NULL);
    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);

    close(sock);

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

void loadLinks(){
    int rot1, rot2, custo;

   memset(adjacencia, 0, sizeof(adjacencia));

    FILE *file = fopen("configs/enlaces.config", "r");
    if (!file)
        die("Não foi possível abrir o arquivo de Enlaces");

    //Lê os 3 valores e salva na matriz
    while (fscanf(file, "%d %d %d", &rot1, &rot2, &custo) != EOF){
        adjacencia[rot1][rot2] = custo;
        adjacencia[rot2][rot1] = custo;
    }
    fclose(file);
}

void loadConfs(int adjacentes[])
{
    int id_rot, porta_rot;
    char ip_rot[32];
    FILE *file;
    roteadores = malloc(sizeof(struct roteador) * n_adj);

    for(int i=0; i<n_adj; i++){
        file = fopen("configs/roteador.config", "r");
        if (!file)
            die("Não foi possível abrir o arquivo de Roteadores");

        while (fscanf(file, "%d %d %s", &id_rot, &porta_rot, ip_rot) != EOF){
            if(adjacentes[i] == id_rot){
                roteadores[i].id    =  id_rot;
                roteadores[i].porta =  porta_rot;
                strcpy(roteadores[i].ip, ip_rot);
            }
        }
        fclose(file);
    }
}

void socketConfig()
{
    //create a UDP socket
    if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(roteadores[0].porta);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if( bind(sock , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
}

void sendPacket(pacote packet)
{
    int id_next, i, slen=sizeof(si_other);

    id_next = caminho[roteadores[0].id][packet.id_dest];

    for(i=1; i<n_adj; i++){
        if (roteadores[i].id == id_next)
            break;
    }

    printf("...encaminhando via roteador: %d | %s:%d\n", id_next, roteadores[i].ip, roteadores[i].porta);

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(roteadores[i].porta);
    
    if (inet_aton(roteadores[i].ip , &si_other.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
        
    if (sendto(sock, &packet, sizeof(struct pacote) , 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
        die("sendto()");
    }
}

void *terminal() 
{
    int i, slen=sizeof(si_other);
    pacote packet;

    while(1)
    {
        printf("Enter router id:\n");
        scanf("%d", &packet.id_dest);
        
        printf("Enter message: ");
        __fpurge(stdin);
        fgets( packet.message, 100, stdin);
        
        packet.seq = ++seq;
        packet.type = DATA;
        packet.ack = 0;
        packet.id_font = roteadores[0].id;

        sendPacket(packet);
        pthread_mutex_lock(&timerMutex);
        tentativa = 0, confirmacao = 0;
        pthread_mutex_unlock(&timerMutex);

        while(1){
            sleep(5);
            pthread_mutex_lock(&timerMutex);
        
            if(tentativa >= 3 || confirmacao){
                pthread_mutex_unlock(&timerMutex);
                break;
            }
            else if(!confirmacao){
                printf("Pacote %d não entregue. Tentando novamente", packet.seq);
                tentativa += 1;
                pthread_mutex_unlock(&timerMutex);
                sendPacket(packet);
            }
        }
        pthread_mutex_unlock(&timerMutex);
    }

    return 0;
}

void *router(void *porta) 
{
    int i, slen = sizeof(si_other) , recv_len;
    int id_destino = -1;
    pacote packet;

    while(1)
    {
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(sock, &packet, sizeof(struct pacote), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        
        id_destino = packet.id_dest;

        sleep(1);
        if (id_destino != roteadores[0].id){
            int id_next = caminho[roteadores[0].id][id_destino];
            
            if(packet.type == DATA){
                printf("Roteador %d encaminhando mensagem com # sequência %d para o destino %d\n", roteadores[0].id, packet.seq, packet.id_dest);
            }else if (packet.type == CONTROL && packet.ack == 1){
                printf("Roteador %d encaminhando confirmação de msg #seq:%d para o sender %d\n", roteadores[0].id, packet.seq, packet.id_dest);
            }
            sendPacket(packet);

        }else if(id_destino == roteadores[0].id && packet.type == DATA){
            pacote response;
            response.type = CONTROL;
            response.ack = 1;
            response.id_font = id_destino;
            response.id_dest = packet.id_font;
            response.seq = packet.seq; // ou ++seq ?

            printf("Pacote recebido de %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            printf("Mensagem: %s\n", packet.message);
            sleep(1);
            puts("Enviando confirmação...");
            
            sendPacket(response);
            
        }else if(id_destino == roteadores[0].id && packet.ack == 1){
            printf("Confirmação recebida de %s:%d, mensagem #seq:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), packet.seq);
            pthread_mutex_lock(&timerMutex);
            confirmacao = 1;
            pthread_mutex_unlock(&timerMutex);
        }
    }
 
    return 0;
}
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include<unistd.h>

#define NODES 6

#define BUFLEN 100

#define SEND 0
#define ROUTE 1

int searchMinor();
void savePath();
void dijkstra();

void loadLinks();
void loadConfs(int adjacentes[]);

void socketConfig();
void sendMessage(int id, char message[], int mode);

void *prompt();
void *router(void *porta);

struct roteador
{
    int id;
    int porta;
    char ip[32];
};

struct pacote
{
    int id;
    char message[BUFLEN];
};

struct roteador *roteadores;
int caminho[NODES][NODES], adjacencia[NODES][NODES];
int n_adj=1;   //n_adj começa com um porque já considera sua própria porta
int sock;
struct sockaddr_in si_me, si_other;

void die(char *s)
{
    perror(s);
    exit(1);
}

void printRoteadores(){
    puts("Roteador atual e vizinhos:");
    for(int i=0; i<n_adj; i++)
        printf("roteador:%d %s:%d\n", roteadores[i].id, roteadores[i].ip, roteadores[i].porta);
}

int main(void)
{
    int *meuid, i, j, adjacentes[NODES];
    meuid = malloc(sizeof(int));
    
    memset(&adjacentes, 0, sizeof(int) * NODES);

    printf("Olá, por favor informe o meu ID: ");
    scanf("%d", meuid);

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
    pthread_create(&tids[1], NULL, prompt, NULL);
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

void loadConfs(int adjacentes[])
{
    int id_rot, porta_rot;
    char ip_rot[32];
    FILE *file;
    roteadores = malloc(sizeof(struct roteador) * n_adj);

    for(int i=0; i<n_adj; i++){
        file = fopen("roteador.config", "r");
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

void sendMessage(int id, char message[], int mode)
{
    // char packet[BUFLEN];
    int id_next, i, slen=sizeof(si_other);
    struct pacote pacote;
    
    if(mode == SEND)
        pacote.id = id;

    strcpy(pacote.message, message);

    id_next = caminho[roteadores[0].id][id];

    for(i=1; i<n_adj; i++){
        if (roteadores[i].id == id_next)
            break;
    }

    printf("route destino %d\n", id_next);
    printf("porta destino %d\n", roteadores[i].porta);

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(roteadores[i].porta);
    
    if (inet_aton(roteadores[i].ip , &si_other.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
        
    if (sendto(sock, &pacote, sizeof(struct pacote) , 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
        die("sendto()");
    }
}

void *prompt() 
{
    int i, slen=sizeof(si_other);
    char buf[BUFLEN];
    char packet[BUFLEN], message[BUFLEN];
    int id_destino;
 
    while(1)
    {
        printf("Enter router id:\n");
        scanf("%d", &id_destino);
        
        printf("Enter message:\n");
        scanf("%s", message);

        sendMessage(id_destino, message, SEND);
    }

    return 0;
}

void *router(void *porta) 
{
    int i, slen = sizeof(si_other) , recv_len;
    char buf[BUFLEN];
    char *token;
    int id_destino = -1;
    struct pacote pacote;

    while(1)
    {
        memset(buf,'\0', BUFLEN);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(sock, &pacote, sizeof(struct pacote), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        
        id_destino = pacote.id;

        if (id_destino != roteadores[0].id){
            int id_next = caminho[roteadores[0].id][id_destino];
            sleep(1);
            printf("Pacote para %d encaminhando por %d...\n", id_destino, id_next);
            sendMessage(id_next, pacote.message, ROUTE);            

        }else{
            sleep(1);
            printf("Pacote recebido de %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            printf("Mensagem: %s\n", pacote.message);
        }
    }
 
    return 0;
}
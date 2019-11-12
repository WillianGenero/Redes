#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include<unistd.h>
#include <stdio_ext.h>
#include "headers/structures.h"

void printaNodo();
void printaVec();
void printaTable();
void printaVizinhos();

int idx(int myid);
void mapeia();
void loadLinks();
void loadConfs(int vizinhos[]);
void socketConfig();
void sendMyVec();
void sendPacket(pacote packet);
void updateTable(pacote packet);
void *terminal();
void *router(void *porta);

struct roteador *roteadores;
struct sockaddr_in si_me, si_other;
pthread_mutex_t timerMutex = PTHREAD_MUTEX_INITIALIZER;
int sock, seq = 0, confirmacao = 0, tentativa = 0;

int nodos[NODES], saida[NODES], qt_nodos = 0, *myvec;
int *table[NODES];

int vizinhos[NODES], n_viz = 1;

void die(char *s)
{
    perror(s);
    exit(1);
}

/* retorna o indice do roteador no vetor */
int idx(int id){
    for(int i=0; i<qt_nodos; i++){
        if(nodos[i] == id)
            return i;
    }
}

void printRoteadores(){
    puts("---------------------------------");
    printf("| id:%d %s:%d\n| Vizinhos:\n", roteadores[0].id, roteadores[0].ip, roteadores[0].porta);
    for(int i=1; i<n_viz; i++)
        printf("| id:%d | %s:%d\n", roteadores[i].id, roteadores[i].ip, roteadores[i].porta);
    puts("---------------------------------");
}

void adicionaVizinho(int id) {
    for(int i=1; i< n_viz; i++){
        // o vizinho já foi descoberto
        if(id == vizinhos[i])
            return;
    }
    vizinhos[n_viz] = id;
    n_viz++;
    return;
}

int *meuid;


int main(int argc, char *argv[ ])
{
    meuid = malloc(sizeof(int));

    if(argc==1){
        printf("Olá, por favor informe o meu ID:\n");
        scanf("%d", meuid);
    }else if(argc==2){
        *meuid = atoi(argv[1]);
        printf("Olá, sou o roteador %d:\n", *meuid);
    }

    mapeia();
    loadLinks(*meuid);

    printaNodo();
    printaTable();
    printaVizinhos();

    loadConfs(vizinhos);

    printRoteadores();


    pthread_t tids[2];

    socketConfig();

    sleep(2);

    sendMyVec();

    pthread_create(&tids[0], NULL, router, (void *) &roteadores[0].porta);
    pthread_create(&tids[1], NULL, terminal, NULL);
    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);

    close(sock);
    return(1);
}

void printaNodo(){
    printf("nodos: ");
    for(int i=0; i<qt_nodos; i++){
        printf("[%d]", nodos[i]);
    }
    puts("");
}

void printaVec(int *vec){
    //printf("meu vetor: ");
    for(int i=0; i<qt_nodos; i++){
        printf("[%d]", vec[i]);
    }
    puts("");
}

void printaTable(){
    puts("\n Tabela:");
    for(int i = 0; i < NODES; i++){
        if(table[i] == -1){
            puts("N/A");
            continue;
        }
        for(int j = 0; j < NODES; j++){
            printf("[%d]", table[i][j]);
        }
        puts("");
    }
    puts("");
}

void printaVizinhos(){
    printf("Vizinhos: ");
    for (int i = 1; i < n_viz; i++){
        printf("[%d]", vizinhos[i]);
    }
    puts("");
}

void mapeia(){
    int rot1, rot2, custo;
    int r1_n, r2_n;

    FILE *file = fopen("configs/enlaces.config", "r");
    if (!file)
        die("Não foi possível abrir o arquivo de Enlaces");

    while (fscanf(file, "%d %d %d", &rot1, &rot2, &custo) != EOF){
        int r1_n = r2_n = 1;
        for(int i=0; i < qt_nodos; i++){
            if (rot1 == nodos[i]){
                r1_n = 0;
            } else if(rot2 == nodos[i]){
                r2_n = 0;
            }
        }
        if(r1_n){
            nodos[qt_nodos] = rot1;
            qt_nodos++;
        }
        if(r2_n){
            nodos[qt_nodos] = rot2;
            qt_nodos++;
        }
    }
    fclose(file);
}

void loadLinks(int myid){
    int rot1, rot2, custo;
    myvec = malloc(sizeof(int) * qt_nodos);

    memset(myvec, -1, sizeof(int) * qt_nodos);
    memset(saida, -1, sizeof(int) * NODES);

    myvec[idx(myid)] = 0;

    FILE *file = fopen("configs/enlaces.config", "r");
    if (!file)
        die("Não foi possível abrir o arquivo de Enlaces");

    vizinhos[0] = myid;

    while (fscanf(file, "%d %d %d", &rot1, &rot2, &custo) != EOF){
        for(int i=0; i < qt_nodos; i++){
            if(rot1 == myid){
                myvec[idx(rot2)] = custo;
                adicionaVizinho(rot2);
            }if(rot2 == myid){
                myvec[idx(rot1)] = custo;
                adicionaVizinho(rot1);
            }
        }
    }
    fclose(file);

    memset(table, -1, sizeof(int*) * qt_nodos);
    table[idx(myid)] = myvec;
}

void loadConfs(int vizinhos[])
{
    int id_rot, porta_rot;
    char ip_rot[32];
    FILE *file;
    roteadores = malloc(sizeof(struct roteador) * n_viz);

    for(int i=0; i<n_viz; i++){
        file = fopen("configs/roteador.config", "r");
        if (!file)
            die("Não foi possível abrir o arquivo de Roteadores");

        while (fscanf(file, "%d %d %s", &id_rot, &porta_rot, ip_rot) != EOF){
            if(vizinhos[i] == id_rot){
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

void sendMyVec()
{
    pacote vec_packet;
    
        vec_packet.id_font = *meuid;
    vec_packet.type = CONTROL;
    vec_packet.ack = 0;    
    
        for (int i=0 ; i<qt_nodos ; i++)
            vec_packet.myvec[i] = myvec[i];

    for (int i = 1; i < n_viz; i++){
        vec_packet.id_dest = vizinhos[i];
        sendPacket(vec_packet);
    }
}

void sendPacket(pacote packet)
{
    int id_next, i, slen=sizeof(si_other);

    id_next = packet.id_dest;

    for(i=1; i<n_viz; i++){
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
        while(1){
            printf("Enter router id:\n");
            scanf("%d", &packet.id_dest);
            if (packet.id_dest == roteadores[0].id)
                printf("Destino não alcançável, tente novamente.\n");
            else
                break;
        }

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
            // int id_next = caminho[roteadores[0].id][id_destino];

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
        }else if (id_destino == roteadores[0].id && packet.type == CONTROL){
            printf("Vetor distância de: %d -> ", packet.id_font);
            printaVec(packet.myvec);
            updateTable(packet);
        }
    }
    return 0;
}
void updateTable(pacote packet)
{
    puts("Atualizar tabela de roteamento");
}

#define BUFLEN 100
#define NODES 4

// packet types
#define DATA 0
#define CONTROL 1

typedef struct roteador
{
    int id;
    int porta;
    char ip[32];
} roteador;

typedef struct pacote
{
    int id_dest;
    int id_font;
    int seq;
    char message[150];
    int type;
    int ack;
    int sendervec[NODES];
} pacote;
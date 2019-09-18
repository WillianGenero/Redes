#include <stdio.h>
#include <stdlib.h>
#define NODES 6
int caminho[NODES][NODES];

void dijkstra();
void carregaEnlaces();

void main(){
    carregaEnlaces();
}

void carregaEnlaces(){
    int adjacencia[NODES][NODES];
    int rot1, rot2, custo;
    //Zerar matriz
   for (int i=0 ; i<NODES ; i++){
        for (int j=0 ; j<NODES ; j++)
            adjacencia[i][j] = 0;
   }
    //Abre arquivo ENLACES
    FILE *file = fopen("enlaces.config", "r");
    if (!file)
        printf("Não foi possível abrir o arquivo de Enlaces");

    //Lê os 3 valores e salva na matriz
    while (fscanf(file, "%d %d %d", &rot1, &rot2, &custo) != EOF){
        adjacencia[rot1-1][rot2-1] = custo;
        adjacencia[rot2-1][rot1-1] = custo;
    }
    fclose(file);

   for (int i=0 ; i<NODES ; i++){
        for (int j=0 ; j<NODES ; j++)
            printf("Posição: %d | %d Custo: %d\n", i, j, adjacencia[i][j]);
   }
}
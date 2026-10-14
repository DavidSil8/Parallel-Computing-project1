#include <stdio.h>
#include <stdlib.h>

int n;
int** matrix;
int readMatrix();
void freeMatrix();
void printMatrix();

int main(int argc, char *argv[]) {
    scanf("%d", &n); //Ler o número de vértices
    if (readMatrix()==-1){printf("Erro na criação da matriz\n"); exit(1);}
    printMatrix();
    freeMatrix();
}

int readMatrix() {
    //Função para ler e alocar espaço para a matriz. Retorna -1 se houver um erro ou 0 caso a alocação funcione

    //Criar a matriz
    matrix = malloc(n * sizeof(int*));
    if (matrix == NULL) {return -1;}
    for (int i = 0; i < n; i++) {
        matrix[i] = malloc(n * sizeof(int));
        if (matrix[i] == NULL) {return -1;}
    }

    //Ler a matriz
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            scanf("%d", &matrix[i][j]);
        }
    }
    return 0;
}

void freeMatrix() {
    //Função para libertar a matrix (foi criada com malloc)

    for (int i = 0; i < n; i++) {
        free(matrix[i]); //Libertamos cada uma das linhas
    }
    free(matrix); //Libertamos o pointer para as linhas
}

void printMatrix() {
    //Função para imprimir a matrix
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d%c", matrix[i][j], " \n"[j==n-1]);//o caracter é um espaço normalmente e um parágrafo no fim da linha (quando j é n-1)
        }
    }
}
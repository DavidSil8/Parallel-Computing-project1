#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int createAndReadMatrix();
void freeMatrix();
void printMatrix();
int foxCalculator(int p);

int n;
int** matrix;

int main(int argc, char *argv[]) {

    int numProcs, rank;

    scanf("%d", &n); //Ler o número de vértices
    createAndReadMatrix();

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int q = foxCalculator(numProcs);
    if (q==0) {
        MPI_Finalize();
        if (rank == 0)
            printf("Dimensão da matriz e número de processos não são compatíveis.\n");
        exit(0);
    }

    MPI_Finalize();

    printMatrix();
    freeMatrix();
}

int createAndReadMatrix() {
    //Função para ler e alocar espaço contíguo para a matriz
    matrix = malloc(n * sizeof(int*));
    int *data = malloc(n * n * sizeof(int));
    for (int i = 0; i < n; i++) matrix[i] = &data[i * n];

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
    free(matrix[0]);
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

int foxCalculator(int p) {
    //Calcula e retorna o q para aplicar o algoritmo de Fox. Se não for possível, retorna 0
    int q=1;
    while (q*q<p) { //Tentamos encontrar um q que seja a raiz quadrada de p
        q++;
    }
    //Agora q*q é igual ou maior do que p
    if (q*q!=p || n%q!=0) {return 0;}
    return q;
}

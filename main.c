#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int** createMatrix(int n);
void readMatrix(int** matrix, int n);
void freeMatrix(int** matrix);
void printMatrix(int** matrix, int n);
int foxCalculator(int n, int p);

int main(int argc, char *argv[]) {
    int n;
    int** matrix;
    int numProcs, rank;

    scanf("%d", &n); //Ler o número de vértices
    matrix = createMatrix(n);
    readMatrix(matrix, n);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int q = foxCalculator(n, numProcs);
    if (q==0) {
        MPI_Finalize();
        if (rank == 0)
            printf("Dimensão da matriz e número de processos não são compatíveis.\n");
        exit(0);
    }

    MPI_Finalize();

    printMatrix(matrix, n);
    freeMatrix(matrix);
}

int** createMatrix(int n) {
    //Aloca espaço para a matriz
    int** matrix = malloc(n * sizeof(int*));
    int *data = malloc(n * n * sizeof(int));
    for (int i = 0; i < n; i++) matrix[i] = &data[i * n];
    return matrix;
}

void readMatrix(int** matrix, int n) {
    //Lê o conteúdo da matriz no input
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            scanf("%d", &matrix[i][j]);
        }
    }
}

void freeMatrix(int** matrix) {
    //Liberta a matrix (foi criada com malloc)
    free(matrix[0]);
    free(matrix);
}

void printMatrix(int** matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d%c", matrix[i][j], " \n"[j==n-1]);//o caracter é um espaço normalmente e um parágrafo no fim da linha (quando j é n-1)
        }
    }
}

int foxCalculator(int n, int p) {
    //Calcula e retorna o q para aplicar o algoritmo de Fox. Se não for possível, retorna 0
    int q=1;
    while (q*q<p) { //Tentamos encontrar um q que seja a raiz quadrada de p
        q++;
    }
    //Agora q*q é igual ou maior do que p
    if (q*q!=p || n%q!=0) {return 0;}
    return q;
}

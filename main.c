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

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank==0) {
        scanf("%d", &n); //Ler o número de vértices
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Vamos verificar se o tamanho da matriz e o número de processos permitem aplicar o algoritmo de fox
    int q = foxCalculator(n, numProcs);
    if (q==0) {
        MPI_Finalize();
        if (rank == 0)
            printf("Dimensão da matriz e número de processos não são compatíveis.\n");
        exit(0);
    }

    matrix = createMatrix(n);
    if (rank==0) {
        readMatrix(matrix, n);
    }

    int subN = n/q;

    //Vamos agora dividir a matriz nas várias sub matrizes
    MPI_Datatype submatrixTypeUnsized, submatrixType;
    MPI_Type_vector(subN, subN, n, MPI_INT, &submatrixTypeUnsized); //Este tipo corresponde à distribuição da submatriz na memória
    /*
     *Para conseguirmos enviar várias submatrizes sem o mpi achar que cada submatriz começa onde a anterior acabou, temos
     *de mudar o tamanho do datatype. Vamos definir que a submatriz ocupa subN ints de espaço. Assim, com um displacement de 1
     *passamos para a segunda submatriz.
     */
    MPI_Type_create_resized(submatrixTypeUnsized,0,subN*sizeof(int),&submatrixType);
    MPI_Type_commit(&submatrixType);

    int sendcounts[numProcs];
    int displacements[numProcs];
    for (int i = 0; i < numProcs; i++) {
        sendcounts[i] = 1; //enviamos 1 elemento do tipo submatrixType para cada processo
        displacements[i] = i%q + (i/q)*q*(subN); //as submatrizes começam em "coordenadas" diferentes da matriz e esta expressão dá-nos o ponto inicial de cada uma
    }
    int** submatrix = createMatrix(subN);

    //Scatterv permite usar displacements
    MPI_Scatterv(matrix[0], sendcounts, displacements, submatrixType, submatrix[0], (subN)*(subN), MPI_INT, 0, MPI_COMM_WORLD);

    //Início da parte alterável

    printf("rank: %d  ",rank);
    printMatrix(submatrix, subN);

    if (rank==0) {
        printMatrix(matrix, n);
    }

    //Fim da parte alterável

    freeMatrix(matrix);
    freeMatrix(submatrix);
    MPI_Type_free(&submatrixType);
    MPI_Finalize();
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

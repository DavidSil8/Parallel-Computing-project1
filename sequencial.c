#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int** createMatrix(int n);
void readMatrix(int** matrix, int n);
void freeMatrix(int** matrix);
void printMatrix(int** matrix, int n);
void fillInfMatrix(int** matrix, int n);
void copyMatrix(int** source, int** dest, int n);
void distProduct(int** matrixA, int** matrixB, int** result, int n);
int specialMin(int a, int b, int current);

int main(int argc, char *argv[]) {
    int n;
    int** matrix;

    MPI_Init(&argc, &argv);

    scanf("%d", &n); //Ler o número de vértices

    //Vamos alocar espaço para a matriz e lê-la do input
    matrix = createMatrix(n);
    readMatrix(matrix, n);


    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime(); //Vamos começar a medição do tempo de execução


    //Vamos também criar uma matriz que vai guardar os resultados das multiplicações
    int** result = createMatrix(n);


    //Agora podemos começar o repeating squaring method
    for (int exponent = 1; exponent < n; exponent*=2) { //A variável exponent diz-nos em que ponto vamos no repeated squaring method
        fillInfMatrix(result, n); //Tem de começar como todos os valores a infinitos para irem reduzindo sempre que é encontrado um novo mínimo

        distProduct(matrix, matrix, result, n); //Fazemos as contas entre as duas matrizes e guardamos o resultado em result

        if (exponent*2<n) { //Em todas as iterações menos na última
            copyMatrix(result,matrix,n); //Para recomeçarmos basta guardar o resultado na matriz base e passar à próxima iteração
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime(); //Acabamos a medição do tempo de execução

    printMatrix(result, n);
    printf("Tempo de execução: %f milissegundos\n", (end - start)*1000);

    //Temos de ter cuidado e libertar a memória que alocámos
    freeMatrix(matrix);
    freeMatrix(result);
    MPI_Finalize();
}

int** createMatrix(int n) {
    //Aloca espaço para uma matriz n*n
    int** matrix = malloc(n * sizeof(int*));
    int *data = malloc(n * n * sizeof(int));
    for (int i = 0; i < n; i++) matrix[i] = &data[i * n];
    return matrix;
}

void readMatrix(int** matrix, int n) {
    //Lê o conteúdo da matriz no input e troca os zeros fora da diagonal principal por infintos para o algoritmo funcionar
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int temp;
            scanf("%d", &temp);
            if (temp==0 && i!=j) {
                temp = INT_MAX; //Zeros fora da diagonal principal devem ser tratados como infinitos
            }
            matrix[i][j] = temp;
        }
    }
}

void freeMatrix(int** matrix) {
    //Liberta a memória que foi alocada para a matriz
    free(matrix[0]);
    free(matrix);
}

void printMatrix(int** matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int temp = matrix[i][j];
            if (temp==INT_MAX) {temp = 0;} //Na representação escrevemos infinitos como zeros
            printf("%d%c", temp, " \n"[j==n-1]);//O %c é um espaço normalmente e um parágrafo no fim da linha (quando j é n-1)
        }
    }
}

void fillInfMatrix(int** matrix, int n) {
    //Enche uma matriz com valores de infinito (INT_MAX)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = INT_MAX;
        }
    }
}

void copyMatrix(int** source, int** dest, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            dest[i][j] = source[i][j];
        }
    }
}

void distProduct(int** matrixA, int** matrixB, int** result, int n) {
    //Multiplica a primeira matriz pela segunda e atualiza o resultado que está na terceira
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                result[i][j] = specialMin(matrixA[i][k], matrixB[k][j], result[i][j]);
            }
        }
    }
}

int specialMin(int a, int b, int current) {
    //Retorna o mínimo entre a+b e current mas evita overflow porque a e/ou b podem ser INT_MAX
    if (a==INT_MAX || b==INT_MAX){return current;}
    if (a+b<current) {return a+b;}
    return current;
}

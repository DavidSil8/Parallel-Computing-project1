#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int** createMatrix(int n);
void readMatrix(int** matrix, int n);
void freeMatrix(int** matrix);
void printMatrix(int** matrix, int n);
int qCalculator(int n, int p);
void fillInfMatrix(int** matrix, int n);
void copyMatrix(int** source, int** dest, int n);
void distProduct(int** matrixA, int** matrixB, int** result, int n);
int specialMin(int a, int b, int current);

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
    int q = qCalculator(n, numProcs);
    if (q==0) {
        MPI_Finalize();
        if (rank == 0)
            printf("Dimensão da matriz e número de processos não são compatíveis.\n");
        exit(0);
    }

    int subN = n/q;

    //Vamos alocar espaço para a matriz e lê-la do input
    matrix = createMatrix(n);
    if (rank==0) {
        readMatrix(matrix, n);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime(); //Vamos começar a medição do tempo de execução


    //Vamos criar um comunicador para cada linha e para cada coluna
    MPI_Comm rowComm, colComm;
    MPI_Comm_split(MPI_COMM_WORLD, rank/q, rank, &rowComm);
    MPI_Comm_split(MPI_COMM_WORLD, rank%q, rank, &colComm);
    //E guardar os ranks nestes novos comunicadores
    int rankRow, rankCol;
    MPI_Comm_rank(rowComm, &rankRow);
    MPI_Comm_rank(colComm, &rankCol);


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
        displacements[i] = i%q + (i/q)*(subN)*q; //as submatrizes começam em "coordenadas" diferentes da matriz e esta expressão dá-nos o ponto inicial de cada uma
    }
    int** subMatrix = createMatrix(subN); //Esta matriz vai servir como a "base" para cada processo
    //Vamos distribuir a matriz pelos processos usando scatterv (permite usar displacements, ao contrário do scatter normal)
    MPI_Scatterv(&matrix[0][0], sendcounts, displacements, submatrixType, &subMatrix[0][0], (subN)*(subN), MPI_INT, 0, MPI_COMM_WORLD);


    //Precisamos de mais algumas matrizes: vamos chamar subMatrixA à matriz da esquerda na multiplicação e subMatrixB à da direita
    int** subMatrixA = createMatrix(subN);
    int** subMatrixB = createMatrix(subN);
    //Vamos também criar uma matriz que vai guardar os resultados das multiplicações
    int** result = createMatrix(subN);


    //Agora podemos começar o repeating squaring method
    for (int exponent = 1; exponent < n; exponent*=2) { //A variável exponent diz-nos em que ponto vamos no repeated squaring method
        fillInfMatrix(result, subN); //Tem de começar como todos os valores a infinitos para irem reduzindo sempre que é encontrado um novo mínimo

        //A subMatrixB começa igual à subMatrix em todos os processos e "viaja" para cima depois de cada iteração
        copyMatrix(subMatrix, subMatrixB, subN);

        //Podemos agora começar a parte iterativa
        for (int i = 0; i < q; i++) {
            int chosenRankRow = (rankCol + i)%q; //Escolhemos um processo de cada linha
            if (rankRow==chosenRankRow) {
                copyMatrix(subMatrix, subMatrixA, subN);
            }
            MPI_Bcast(&subMatrixA[0][0], subN*subN, MPI_INT, chosenRankRow, rowComm); //O processo escolhido partilha a sua subMatrix com os da sua linha
            distProduct(subMatrixA, subMatrixB, result, subN); //Fazemos as contas entre as duas matrizes e guardamos o resultado em result
            //Temos de mandar a subMatrixB para o processo acima na mesma coluna
            MPI_Sendrecv_replace(&subMatrixB[0][0],subN*subN,MPI_INT,(rankCol-1+q)%q,0,(rankCol+1)%q,0,colComm,MPI_STATUS_IGNORE);
            //Na destination somámos q antes de usar % porque % não funciona da mesma forma com números negativos
        }

        if (exponent*2<n) { //Em todas as iterações menos na última
            copyMatrix(result,subMatrix,subN); //Para recomeçarmos basta guardar o resultado na matriz base e passar à próxima iteração
        }
    }

    //Temos agora de juntar as matrizes result numa só matriz. Para isso podemos reutilizar os sendcounts e os displacements do scatterv
    MPI_Gatherv(&result[0][0], subN*subN, MPI_INT, &matrix[0][0], sendcounts, displacements, submatrixType, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime(); //Acabamos a medição do tempo de execução

    if (rank==0) {
        printMatrix(matrix, n);
        printf("Tempo de execução: %f milissegundos\n", (end - start)*1000);
    }

    //Temos de ter cuidado e libertar a memória que alocámos
    freeMatrix(matrix);
    freeMatrix(subMatrix);
    freeMatrix(subMatrixA);
    freeMatrix(subMatrixB);
    freeMatrix(result);
    MPI_Type_free(&submatrixTypeUnsized);
    MPI_Type_free(&submatrixType);
    MPI_Comm_free(&rowComm);
    MPI_Comm_free(&colComm);
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

int qCalculator(int n, int p) {
    //Calcula e retorna o q para aplicar o algoritmo de Fox. Se não for possível, retorna 0
    int q=1;
    while (q*q<p) { //Tentamos encontrar um q que seja a raiz quadrada de p
        q++;
    }
    //Agora q*q é igual ou maior do que p
    if (q*q!=p || n%q!=0) {return 0;}
    return q;
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

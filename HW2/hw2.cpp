#include <pthread.h>
#include <semaphore.h>
#include <iostream>

int main(){
    int A_rows, A_cols, B_rows, B_cols, C_rows, C_cols, D_rows, D_cols;
    //TAKE INPUT
    std::cin >> A_rows >> A_cols;
    int A[A_rows][A_cols];
    for(int i = 0; i < A_rows; i++){
        for(int j = 0; j < A_cols; j++){
            std::cin >> A[i][j];
        }
    }
    std::cin >> B_rows >> B_cols;
    int B[B_rows][B_cols];
    for(int i = 0; i < B_rows; i++){
        for(int j = 0; j < B_cols; j++){
            std::cin >> B[i][j];
        }
    }
    std::cin >> C_rows >> C_cols;
    int C[C_rows][C_cols];
    for(int i = 0; i < C_rows; i++){
        for(int j = 0; j < C_cols; j++){
            std::cin >> C[i][j];
        }
    }
    std::cin >> D_rows >> D_cols;
    int D[D_rows][D_cols];
    for(int i = 0; i < D_rows; i++){
        for(int j = 0; j < D_cols; j++){
            std::cin >> D[i][j];
        }
    }

    



    return 0;
}
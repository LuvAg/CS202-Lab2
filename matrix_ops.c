
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX 10
#define FNAME_SZ 128

/* Helper: safe integer input with prompt */
int safe_int_read(const char *prompt) {
    int x;
    int ok;
    while (1) {
        printf("%s", prompt);
        ok = scanf("%d", &x);
        if (ok == 1) {
            /* consume newline */
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            return x;
        } else {
            printf("Invalid input. Please enter an integer.\n");
            /* clear input */
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
        }
    }
}

/* Print matrix */
void printMatrix(int a[MAX][MAX], int r, int c) {
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < c; ++j) {
            printf("%6d ", a[i][j]);
        }
        printf("\n");
    }
}

/* Read matrix from stdin */
void readMatrix(int a[MAX][MAX], int r, int c) {
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < c; ++j) {
            char prompt[64];
            snprintf(prompt, sizeof(prompt), "Enter element [%d][%d]: ", i, j);
            a[i][j] = safe_int_read(prompt);
        }
    }
}

/* Fill matrix with random values in range [-range,range] */
void randomFill(int a[MAX][MAX], int r, int c, int range) {
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            a[i][j] = (rand() % (2*range + 1)) - range;
}

/* Save matrix to a text file (r c then rows) */
int saveMatrixToFile(const char *fname, int a[MAX][MAX], int r, int c) {
    FILE *fp = fopen(fname, "w");
    if (!fp) return -1;
    fprintf(fp, "%d %d\n", r, c);
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < c; ++j) {
            fprintf(fp, "%d ", a[i][j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    return 0;
}

/* Load matrix from file */
int loadMatrixFromFile(const char *fname, int a[MAX][MAX], int *r, int *c) {
    FILE *fp = fopen(fname, "r");
    if (!fp) return -1;
    if (fscanf(fp, "%d %d", r, c) != 2) {
        fclose(fp);
        return -2;
    }
    if (*r < 0 || *r > MAX || *c < 0 || *c > MAX) {
        fclose(fp);
        return -3;
    }
    for (int i = 0; i < *r; ++i) {
        for (int j = 0; j < *c; ++j) {
            if (fscanf(fp, "%d", &a[i][j]) != 1) {
                fclose(fp);
                return -4;
            }
        }
    }
    fclose(fp);
    return 0;
}

/* Addition and subtraction */
void addMatrix(int a[MAX][MAX], int b[MAX][MAX], int res[MAX][MAX], int r, int c) {
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            res[i][j] = a[i][j] + b[i][j];
}

void subMatrix(int a[MAX][MAX], int b[MAX][MAX], int res[MAX][MAX], int r, int c) {
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            res[i][j] = a[i][j] - b[i][j];
}

/* Multiply a (r1 x c1) by b (c1 x c2) into result (r1 x c2) */
void multMatrix(int a[MAX][MAX], int b[MAX][MAX], int res[MAX][MAX], int r1, int c1, int c2) {
    for (int i = 0; i < r1; ++i) {
        for (int j = 0; j < c2; ++j) {
            res[i][j] = 0;
            for (int k = 0; k < c1; ++k) {
                res[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

/* Scalar multiply */
void scalarMultiply(int a[MAX][MAX], int res[MAX][MAX], int r, int c, int scalar) {
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            res[i][j] = a[i][j] * scalar;
}

/* Transpose (r x c -> c x r) */
void transpose(int a[MAX][MAX], int res[MAX][MAX], int r, int c) {
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            res[j][i] = a[i][j];
}

/* Helper: copy matrix */
void copyMatrix(int src[MAX][MAX], int dst[MAX][MAX], int r, int c) {
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            dst[i][j] = src[i][j];
}

/* Determinant: compute recursively using expansion by minors.
   Not optimized; works for small n (<=8 realistically). */
int determinant_recursive(int mat[MAX][MAX], int n) {
    if (n == 1) {
        return mat[0][0];
    }
    if (n == 2) {
        return mat[0][0]*mat[1][1] - mat[0][1]*mat[1][0];
    }
    int det = 0;
    int temp[MAX][MAX];
    for (int p = 0; p < n; p++) {
        int subi = 0;
        for (int i = 1; i < n; i++) {
            int subj = 0;
            for (int j = 0; j < n; j++) {
                if (j == p) continue;
                temp[subi][subj] = mat[i][j];
                subj++;
            }
            subi++;
        }
        int sign = (p % 2 == 0) ? 1 : -1;
        det += sign * mat[0][p] * determinant_recursive(temp, n-1);
    }
    return det;
}

/* Pretty header for menu */
void printHeader(const char *title) {
    printf("\n================ %s ================\n", title);
}

/* Validate dimensions for operations */
int dims_equal(int r1, int c1, int r2, int c2) {
    return r1 == r2 && c1 == c2;
}

/* Main menu for the matrix program */
int main(void) {
    int A[MAX][MAX] = {0}, B[MAX][MAX] = {0}, R[MAX][MAX] = {0};
    int r1 = 0, c1 = 0, r2 = 0, c2 = 0;
    int choice;
    char fname[FNAME_SZ];
    srand((unsigned)time(NULL));

    printHeader("Matrix Operations - Extended");

    /* Ask user to choose whether to load or input matrices */
    while (1) {
        printf("\nInitial setup:\n");
        printf("1. Enter sizes and elements for matrices A and B manually\n");
        printf("2. Random fill matrices A and B\n");
        printf("3. Load matrices from files\n");
        printf("4. Exit program\n");
        int init_choice = safe_int_read("Choice: ");
        if (init_choice == 1) {
            r1 = safe_int_read("Enter rows for matrix A (1..10): ");
            c1 = safe_int_read("Enter cols for matrix A (1..10): ");
            if (r1 <= 0 || r1 > MAX || c1 <= 0 || c1 > MAX) {
                printf("Invalid dimensions. Try again.\n");
                continue;
            }
            printf("Enter Matrix A:\n");
            readMatrix(A, r1, c1);

            r2 = safe_int_read("Enter rows for matrix B (1..10): ");
            c2 = safe_int_read("Enter cols for matrix B (1..10): ");
            if (r2 <= 0 || r2 > MAX || c2 <= 0 || c2 > MAX) {
                printf("Invalid dimensions for B. Try again.\n");
                continue;
            }
            printf("Enter Matrix B:\n");
            readMatrix(B, r2, c2);
            break;
        } else if (init_choice == 2) {
            r1 = safe_int_read("Enter rows for matrix A (1..10): ");
            c1 = safe_int_read("Enter cols for matrix A (1..10): ");
            r2 = safe_int_read("Enter rows for matrix B (1..10): ");
            c2 = safe_int_read("Enter cols for matrix B (1..10): ");
            int rng = safe_int_read("Enter random range (positive integer): ");
            if (rng < 0) rng = 10;
            randomFill(A, r1, c1, rng);
            randomFill(B, r2, c2, rng);
            printf("Matrices random-filled.\n");
            break;
        } else if (init_choice == 3) {
            printf("Enter file name for matrix A: ");
            if (scanf("%127s", fname) != 1) { printf("Read error.\n"); continue; }
            int ret = loadMatrixFromFile(fname, A, &r1, &c1);
            if (ret != 0) { printf("Failed to load A from %s (err=%d)\n", fname, ret); continue; }
            printf("Enter file name for matrix B: ");
            if (scanf("%127s", fname) != 1) { printf("Read error.\n"); continue; }
            ret = loadMatrixFromFile(fname, B, &r2, &c2);
            if (ret != 0) { printf("Failed to load B from %s (err=%d)\n", fname, ret); continue; }
            printf("Loaded A (%d x %d) and B (%d x %d)\n", r1, c1, r2, c2);
            break;
        } else if (init_choice == 4) {
            printf("Goodbye.\n");
            return 0;
        } else {
            printf("Try again.\n");
        }
    }

    /* Main operation loop */
    while (1) {
        printHeader("Main Menu");
        printf("A: dims A = %d x %d | B: dims B = %d x %d\n", r1, c1, r2, c2);
        printf("1. Print matrices\n");
        printf("2. Add (A+B)\n");
        printf("3. Subtract (A-B)\n");
        printf("4. Multiply (A*B)\n");
        printf("5. Scalar multiply A\n");
        printf("6. Transpose A\n");
        printf("7. Determinant of A (must be square)\n");
        printf("8. Save a matrix to file\n");
        printf("9. Load matrix from file into A or B\n");
        printf("10. Swap A and B\n");
        printf("11. Re-enter matrices\n");
        printf("12. Exit\n");

        choice = safe_int_read("Enter choice: ");

        if (choice == 1) {
            printf("\nMatrix A:\n");
            printMatrix(A, r1, c1);
            printf("\nMatrix B:\n");
            printMatrix(B, r2, c2);
        } else if (choice == 2) {
            if (!dims_equal(r1, c1, r2, c2)) {
                printf("Dimensions must be equal for addition.\n");
            } else {
                addMatrix(A, B, R, r1, c1);
                printf("Result (A+B):\n");
                printMatrix(R, r1, c1);
            }
        } else if (choice == 3) {
            if (!dims_equal(r1, c1, r2, c2)) {
                printf("Dimensions must be equal for subtraction.\n");
            } else {
                subMatrix(A, B, R, r1, c1);
                printf("Result (A-B):\n");
                printMatrix(R, r1, c1);
            }
        } else if (choice == 4) {
            if (c1 != r2) {
                printf("For multiplication A(c1) must equal B(r2).\n");
            } else {
                multMatrix(A, B, R, r1, c1, c2);
                printf("Result (A*B):\n");
                printMatrix(R, r1, c2);
            }
        } else if (choice == 5) {
            int scalar = safe_int_read("Enter scalar: ");
            scalarMultiply(A, R, r1, c1, scalar);
            printf("Result (scalar * A):\n");
            printMatrix(R, r1, c1);
        } else if (choice == 6) {
            transpose(A, R, r1, c1);
            printf("Transpose of A (size %d x %d):\n", c1, r1);
            printMatrix(R, c1, r1);
        } else if (choice == 7) {
            if (r1 != c1) {
                printf("Determinant defined only for square matrices.\n");
            } else {
                if (r1 > 8) {
                    printf("Matrix too large for naive det routine (limit = 8).\n");
                } else {
                    int det = determinant_recursive(A, r1);
                    printf("Determinant of A is %d\n", det);
                }
            }
        } else if (choice == 8) {
            printf("Which matrix to save? (A/B): ");
            char ch;
            if (scanf(" %c", &ch) != 1) { printf("Read error\n"); continue; }
            printf("Enter filename: ");
            if (scanf("%127s", fname) != 1) { printf("Read error\n"); continue; }
            int ret;
            if (ch == 'A' || ch == 'a') ret = saveMatrixToFile(fname, A, r1, c1);
            else ret = saveMatrixToFile(fname, B, r2, c2);
            if (ret == 0) printf("Saved successfully.\n");
            else printf("Failed to save.\n");
        } else if (choice == 9) {
            printf("Load into which matrix? (A/B): ");
            char ch;
            if (scanf(" %c", &ch) != 1) { printf("Read error\n"); continue; }
            printf("Enter filename: ");
            if (scanf("%127s", fname) != 1) { printf("Read error\n"); continue; }
            int rr, cc;
            int ret = loadMatrixFromFile(fname, R, &rr, &cc);
            if (ret != 0) {
                printf("Failed to load (err=%d)\n", ret);
            } else {
                if (ch == 'A' || ch == 'a') {
                    r1 = rr; c1 = cc; copyMatrix(R, A, r1, c1);
                } else {
                    r2 = rr; c2 = cc; copyMatrix(R, B, r2, c2);
                }
                printf("Loaded successfully into %c\n", ch);
            }
        } else if (choice == 10) {
            int tempA[MAX][MAX];
            int tempB[MAX][MAX];
            int tr = r1, tc = c1;
            copyMatrix(A, tempA, r1, c1);
            copyMatrix(B, tempB, r2, c2);
            copyMatrix(tempA, B, r1, c1); /* B = old A (but sizes differ perhaps) */
            copyMatrix(tempB, A, r2, c2);
            /* Swap dims */
            r1 = r2; c1 = c2;
            r2 = tr; c2 = tc;
            printf("Swapped A and B.\n");
        } else if (choice == 11) {
            /* Re-enter matrices entirely */
            r1 = safe_int_read("Enter rows for matrix A (1..10): ");
            c1 = safe_int_read("Enter cols for matrix A (1..10): ");
            if (r1 <= 0 || r1 > MAX || c1 <= 0 || c1 > MAX) { printf("Invalid dims\n"); continue; }
            printf("Enter Matrix A:\n");
            readMatrix(A, r1, c1);
            r2 = safe_int_read("Enter rows for matrix B (1..10): ");
            c2 = safe_int_read("Enter cols for matrix B (1..10): ");
            if (r2 <= 0 || r2 > MAX || c2 <= 0 || c2 > MAX) { printf("Invalid dims\n"); continue; }
            printf("Enter Matrix B:\n");
            readMatrix(B, r2, c2);
        } else if (choice == 12) {
            printf("Exiting program.\n");
            break;
        } else {
            printf("Invalid option. Try again.\n");
        }
    }

    return 0;
}

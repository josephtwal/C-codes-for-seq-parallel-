#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include<time.h>


// creadting a fuction to read from matrix from file
float** readmatrix(int* rows, int* cols, const char* filename, int number)
{
    if (rows == NULL || cols == NULL || filename == NULL)
        return NULL;

    *rows = 0;
    *cols = 0;

    FILE* fp = fopen(filename, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "could not open %s: %s\n", filename, strerror(errno));
        return NULL;
    }

    float** matrix = NULL, ** tmp;

    char line[950000];

    while (fgets(line, sizeof line, fp))
    {
        if (*cols == 0)
        {
            // determine the size of the columns based on the first row
            // while loop to read each line 
            char* scan = line;
            int dummy;
            int offset = 0;
            while (sscanf(scan, "%f%n", &dummy, &offset) == 1)
            {
                scan += offset;
                (*cols)++;
            }
        }
        // reallocate the memory size
        tmp = realloc(matrix, (*rows + 1) * sizeof * matrix);

        if (tmp == NULL)
        {
            fclose(fp);
            return matrix; // return all you've parsed so far
        }

        matrix = tmp;
        // allocate the matrix in the memory
        matrix[*rows] = calloc(*cols, sizeof * matrix[*rows]);

        if (matrix[*rows] == NULL)
        {
            fclose(fp);
            if (*rows == 0) // failed in the first row, free everything
            {
                fclose(fp);
                free(matrix);
                return NULL;
            }

            return matrix; // return all you've parsed so far
        }

        int offset = 0;
        char* scan = line;
        for (int j = 0; j < *cols; ++j)
        {
            if (sscanf(scan, "%f%n", matrix[*rows] + j, &offset) == 1)
                scan += offset;
            else
                matrix[*rows][j] = 0; // could not read, set cell to 0
        }

        // incrementing rows
        (*rows)++;
    }

    fclose(fp);

    if (*rows == number && *rows == *cols)
    {
        return matrix;
    }

    else if (*rows >= number && number <= *cols)
    {
        *rows = number;
        *cols = number;
        int x, y;
        float** tmp_matrix = malloc(sizeof(float*) * number);
        for (int i = 0; i < number; ++i)
            tmp_matrix[i] = malloc(sizeof(float) * number);

#pragma omp parallel shared(tmp_matrix,matrix) private(x,y)
        {
#pragma omp for  schedule(static,8)
                for (x = 0; x < number; x++)
                    for (y = 0; y < number; y++)
                    tmp_matrix[x][y] = matrix[x][y];

        }
        return tmp_matrix;
    }

    else
    {
        return matrix = NULL;
    }
}

int main(int argc, char* argv[])
{
    
    // input parameters 

    int nbrows = 0;
    char w[2000];
    char C[2000];
    float* sum;
    float* avg;
    int cols, rows, i, j;

    

    if (argc < 2)
    {
       
        printf("Input file:\n");
        scanf("%s", &C);
        printf("\nOutput file:\n");
        scanf("%s", &w);
        printf("\nMatrix size:\n");
        scanf("%d", &nbrows);
    }
    else
    {
        strcpy(C, argv[1]);
        strcpy(w, argv[2]);
        nbrows = atoi(argv[3]);


        if (!w)
        {
            printf("output file not found");
            return 1;
        }

        if (!C)
        {
            printf("Input file not found");
            return 1;
        }


    }
    
    
    double time_spent = 0.0;
    double time_spent_para=0.0;
    clock_t begin = clock();

    FILE* pf = fopen(&w, "w");

    fprintf(pf, "Program description:\nThis program reads NxN matrix and calculates the average of the numbers in a row and returns the smallest average.\n\n");

    float** matrix = readmatrix(&rows, &cols, &C, nbrows);
    sum = (float*)malloc(rows * sizeof(float));
    avg = (float*)malloc(rows * sizeof(float));


    if (rows == cols)
    {
        if (matrix == NULL)
        {
            //fprintf(stderr, "could not read matrix\n");
            fprintf(pf, "could not read matrix\n");
            return 1;

        }

       /* //printing the matrix 
#pragma omp for private (i,j) 
            for (i = 0; i < rows; ++i)
            {
                for (int j = 0; j < cols; ++j)
                    fprintf(pf, "%.6f\t ", matrix[i][j]);
                fprintf(pf, "\n");
            }
        */

        // finding the sum of each row
        clock_t begin_parallel = clock(); 
#pragma omp parallel shared (sum,avg) private (i,j)
        {
        #pragma omp for schedule(static,8)
            for (i = 0; i < rows; i++)
            {
                sum[i] = 0;
                for (j = 0; j < cols; j++)
                    sum[i] += matrix[i][j];
            }

            /*printing the sum of each row
            fprintf(pf, "\n the sum of each row= ");
            for (int i = 0; i < rows; i++)
                fprintf(pf, "%f  ", sum[i]);
            */

            // finding the average of sum row vector 
        #pragma omp for schedule(static,8)
            for (i = 0; i < rows; i++)
                avg[i] = sum[i] / cols;
        }
        /*printing the average of sum row vector
        fprintf(pf, "\n \n the avg of each row= ");
        for (int i = 0; i < rows; i++)
            fprintf(pf, "%f  ", avg[i]);
            */

        // finding the min of average
        float minAvg = avg[0];

#pragma omp parallel shared(minAvg, avg) private (i)
        {
#pragma omp  for schedule(static,2)
            for (i = 1; i < rows; i++)
            {
                if (minAvg > avg[i])
                    minAvg = avg[i];
            }

        }

        fprintf(pf,"\n\nResult:\n\n*************************\nMinimum average= %f\n*************************", minAvg);
        clock_t end_Parallel = clock();
        time_spent_para += (double)( end_Parallel - begin_parallel) / CLOCKS_PER_SEC;
        printf("\n--- The execution time was : %lf", time_spent_para);
        // freeing memory
        for (int i = 0; i < rows; ++i)
        {
            free(matrix[i]);
        }

        free(matrix);
        free(avg);
        free(sum);

    }

    else
    {
        fprintf(pf, "\nSorry!\nThis program for NxN matrix");
    }

    fclose(pf);
    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("\n--- The execution time was : %lf", time_spent);
    return 0;
}

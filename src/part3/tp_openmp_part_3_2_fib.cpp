/*
**  HISTORY: Written by Tim Mattson, Nov 1999.
*            Modified and extended by Jonathan Rouzaud-Cornabas, Oct 2022
*/

#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <iostream>
#include <omp.h>
#include <fstream>
#include <iomanip>

using namespace std;

static int N = 5;
int num_threads = 1;

#ifndef FS
#define FS 38
#endif

struct node
{
   int data;
   int fibdata;
   struct node *next;
};

void write_perf_csv(int nb_threads, int n, double runtime)
{
   ofstream myfile;
   myfile.open("stats_part3.csv", ios_base::app);
   myfile.precision(8);
   myfile << "3_2 parallelized n r"
          << "," << nb_threads << "," << n << "," << runtime << "\n";

   myfile.close();
}

//
#define CUTOFF 5
int fib_s(int n)
{
   if (n < 2)
      return n;
   int res, a, b;
   a = fib_s(n - 1);
   b = fib_s(n - 2);
   res = a + b;
   return res;
}
int fib_m(int n, int co)
{
   if (co >= CUTOFF)
      return fib_s(n);
   if (n < 2)
      return n;
   int res, a, b;
   co++;
   #pragma omp task shared(a)
   a = fib_m(n - 1, co);
   #pragma omp task shared(b)
   b = fib_m(n - 2, co);
   #pragma omp taskwait
   res = a + b;
   return res;
}

void processwork(struct node *p)
{
   int n;
   n = p->data;
   p->fibdata = fib_m(n, 1);
}

struct node *init_list(struct node *p)
{
   int i;
   struct node *head = NULL;
   struct node *temp = NULL;

   head = (struct node *)malloc(sizeof(struct node));
   p = head;
   p->data = FS;
   p->fibdata = 0;
   for (i = 0; i < N; i++)
   {
      temp = (struct node *)malloc(sizeof(struct node));
      p->next = temp;
      p = temp;
      p->data = FS + i + 1;
      p->fibdata = i + 1;
   }
   p->next = NULL;
   return head;
}

int main(int argc, char *argv[])
{
   // Read command line arguments.
   for (int i = 0; i < argc; i++)
   {
      if ((strcmp(argv[i], "-N") == 0) || (strcmp(argv[i], "-num_node") == 0))
      {
         N = atoi(argv[++i]);
         printf("  User num_node is %d\n", N);
      }
      else if ((strcmp(argv[i], "-T") == 0))
      {
         num_threads = atoi(argv[++i]);
         printf("  User num_threads is %d\n", N);
         omp_set_num_threads(num_threads);
            }
      else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-help") == 0))
      {
         printf("  Fib Options:\n");
         printf("  -num_node (-N) <int>:      Number of node computing fibonnaci numbers (by default 5)\n");
         printf("  -help (-h):            print this message\n\n");
         exit(1);
      }
   }

   struct node *p = NULL;
   struct node *temp = NULL;
   struct node *head = NULL;

   printf("Process linked list\n");
   printf("  Each linked list node will be processed by function 'processwork()'\n");
   printf("  Each ll node will compute %d fibonacci numbers beginning with %d\n", N, FS);

   p = init_list(p);
   head = p;

   // Timer products.
   struct timeval begin, end;

   gettimeofday(&begin, NULL);
   #pragma omp parallel
   {

      #pragma omp single
      {
         while (p != NULL)
         {

            #pragma omp task firstprivate(p)
            {
               processwork(p);
            }
            p = p->next;
         }
      }
   }

   gettimeofday(&end, NULL);

   // Calculate time.
   double time = 1.0 * (end.tv_sec - begin.tv_sec) +
                 1.0e-6 * (end.tv_usec - begin.tv_usec);

   p = head;
   while (p != NULL)
   {
      printf("%d : %d\n", p->data, p->fibdata);
      temp = p->next;
      free(p);
      p = temp;
   }
   free(p);

   printf("Compute Time: %f seconds\n", time);
   write_perf_csv(num_threads, N, time);

   return 0;
}

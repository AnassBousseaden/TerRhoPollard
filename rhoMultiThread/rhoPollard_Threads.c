// Pour compiler : gcc -g -Wall TER.c -lgmp

#include <assert.h>
#include <gmp.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "set.h"

#define SIZE_P 256
#define N 24
#define SIZE_DISTINGUE 239
// en moyenne on stock (cst*sqrt(q))/2^x élement dans ta la table ou
//(x= Nb de bit de p -size distingué)
// Car theta, la proprotion de distingué qu'on sotck
// c'est 1/(2^x) car on stock que les élement qui se code sur size_distingué
// On a en moyenne (cst*sqrt(q)) itération (c'est la complexité de la méthode)
// donc on prend le nombre d'itération et en moyenne on stock une proporation
// theta des résultat
#define N_THREADS 20
#define I_THETA \
  (1 << (SIZE_P - SIZE_DISTINGUE))  // Nb de bit de p -size distingué

#define INIT(name, value) \
  mpz_init(name);         \
  mpz_set_str(name, value, 10);

#define PRINT2(var)             \
  mpz_out_str(stdout, 10, var); \
  printf("\n");

typedef struct {
  mpz_t m; /*exposant de  g*/
  mpz_t n; /*exposant de  h*/
} couple;

/* compteur de la taille du l'arbre */

int cpt = 0;

/* the struct shared by all threads*/
typedef struct {
  node *set;                   /*Ensemble ou les thread mettent les éléments*/
  couple collision_1;          /* element 1 de la collision */
  couple collision_2;          /* element 2 de la collision */
  pthread_mutex_t lock_set;    /* lock pour la table (set) et collision1/2 */
  pthread_mutex_t lock_signal; /* lock pour la condition variable et
                                  collision_1/collision_2 */
  pthread_cond_t signalFinish; /* indique quand la collision a été trouvé  */
  mpz_t p;
  mpz_t q;
  mpz_t g;
  mpz_t h;
  mpz_t listM[N];
  couple listExposant[N];
} thread_struct;

void f(mpz_t *y, mpz_t *a, mpz_t *b, mpz_t *g, mpz_t *h, mpz_t *q, mpz_t *p,
       couple *listExposant, mpz_t *listM) {
  /* on génère une partition de l'ensemble en N cas */
  unsigned long int s = mpz_fdiv_ui(*y, N);
  mpz_add(*b, *b, listExposant[s].n);
  mpz_mod(*b, *b, *q);
  mpz_add(*a, *a, listExposant[s].m);
  mpz_mod(*a, *a, *q);
  mpz_mul(*y, *y, listM[s]);
  mpz_mod(*y, *y, *p);
}

void *f_dinstingue(thread_struct *s_struct) {
  /* INIT */
  mpz_t y, a, b, n1, m1, tmp1, tmp2;
  mpz_t *listExposant, listM;
  mpz_init(tmp1);
  mpz_init(tmp2);
  mpz_init(a);
  mpz_init(b);
  mpz_init(y);
  mpz_init(n1);
  mpz_init(m1);
  gmp_randstate_t state;
  gmp_randinit_mt(state);
  node *node_Tmp;
  int borne = 20 * I_THETA;

start:
  mpz_urandomm(n1, state, s_struct->q);
  mpz_urandomm(m1, state, s_struct->q);
  mpz_powm(tmp1, s_struct->g, m1, s_struct->p); /* tmp1 = g^m1 (mod p)*/
  mpz_powm(tmp2, s_struct->h, n1, s_struct->p); /* tmp1 = h^n1 (mod p)*/
  mpz_mul(y, tmp1, tmp2);
  mpz_set(a, m1);
  mpz_set(b, n1);

  for (int i = 0; i < borne; i++) {
    /*faire un pas avec la fonction f */
    f(&y, &a, &b, &(s_struct->g), &(s_struct->h), &(s_struct->q),
      &(s_struct->p), s_struct->listExposant, s_struct->listM);
    // printf("SIZE OF THE POINT : %d \n here is Y :", mpz_sizeinbase(y, 2));
    // PRINT2(y)
    if (mpz_sizeinbase(y, 2) <= SIZE_DISTINGUE) {
      pthread_mutex_lock(&s_struct->lock_set);
      node_Tmp = search(s_struct->set, y);
      if (node_Tmp != NULL) {
        if (mpz_cmp(b, node_Tmp->n) == 0) {
          pthread_mutex_unlock(&s_struct->lock_set);
          goto start;
        }
        printf("FINISH\n");
        mpz_set(s_struct->collision_1.m, a);
        mpz_set(s_struct->collision_1.n, b);
        mpz_set(s_struct->collision_2.n, node_Tmp->n);
        mpz_set(s_struct->collision_2.m, node_Tmp->m);
        pthread_mutex_lock(&s_struct->lock_signal);
        pthread_cond_broadcast(&s_struct->signalFinish);
        pthread_mutex_unlock(&s_struct->lock_signal);
        printf("CONDITION SIGNALED \n");
        return NULL;
      }
      s_struct->set = insert(s_struct->set, y, b, a);
      cpt++;
      pthread_mutex_unlock(&s_struct->lock_set);
    }
  }
  goto start;
  return NULL;
}

int main(int argc, char *argv[]) {
  bool echec = false;
  FILE *in = fopen("../input.txt", "r");
  FILE *out =
      fopen("output.txt", "w");  // w->a permet de re ecrire a la fin du fichier

  if (in == NULL) {
    printf("Error open file IN\n");
    fclose(out);
    exit(EXIT_FAILURE);
  }
  if (out == NULL) {
    printf("Error open file OUT\n");
    fclose(in);
    exit(EXIT_FAILURE);
  }
  char temp_g[1000];
  char temp_h[1000];
  char temp_p[1000];
  char temp_q[1000];

  // while (!feof(in))
  //{
  struct timeval start, end;
  gettimeofday(&start, NULL);

  fscanf(in, "%s %s %s %s", &temp_g, &temp_h, &temp_p, &temp_q);

  thread_struct sharedStruct; /* shared struct */
  sharedStruct.set = NULL;
  /* génerateur g de G */
  INIT(sharedStruct.g, temp_g)
  /* h tel que l'on cherche log_g(h) */
  INIT(sharedStruct.h, temp_h)
  /* l'ordre du grand groupe */
  INIT(sharedStruct.p, temp_p)
  /* ordre du sous-groupe G */
  INIT(sharedStruct.q, temp_q)

  PRINT2(sharedStruct.h)
  PRINT2(sharedStruct.p)
  PRINT2(sharedStruct.q)

  /* generation des ms et ns de façon aléatoire */
  gmp_randstate_t state;
  gmp_randinit_mt(state);
  mpz_t g_init;
  mpz_t h_init;
  INIT(g_init, "0")
  INIT(h_init, "0")

  for (int i = 0; i < N; i++) {
    mpz_init(sharedStruct.listExposant[i].m);
    mpz_urandomm(sharedStruct.listExposant[i].m, state,
                 sharedStruct.q); /* defined mod q */
    mpz_init(sharedStruct.listExposant[i].n);
    mpz_urandomm(sharedStruct.listExposant[i].n, state, sharedStruct.q);
    mpz_powm(g_init, sharedStruct.g, sharedStruct.listExposant[i].m,
             sharedStruct.p);
    mpz_powm(h_init, sharedStruct.h, sharedStruct.listExposant[i].n,
             sharedStruct.p);
    mpz_init(sharedStruct.listM[i]);
    mpz_mul(sharedStruct.listM[i], g_init, h_init);
  }

  mpz_clear(g_init);
  mpz_clear(h_init);
  gmp_randclear(state);
  /**/

  /* INIT collision*/
  mpz_init(sharedStruct.collision_1.n);
  mpz_init(sharedStruct.collision_2.n);
  mpz_init(sharedStruct.collision_1.m);
  mpz_init(sharedStruct.collision_2.m);

  /* creation de 2 thread qui calcul des points distingué */
  pthread_t tid[N_THREADS];                         /* thread identifiers */
  pthread_mutex_init(&sharedStruct.lock_set, NULL); /* mutex lock for set */
  /* the main thread waits on this variable to shut down everything
  (has to be linked with a mutex) */
  pthread_mutex_init(&sharedStruct.lock_signal, NULL);
  pthread_cond_init(&sharedStruct.signalFinish, NULL);
  /*locking the threads while creating them*/
  pthread_mutex_lock(&sharedStruct.lock_signal);
  pthread_mutex_lock(&sharedStruct.lock_set);

  for (int i = 0; i < N_THREADS; i++) {
    if (pthread_create(&tid[i], NULL, f_dinstingue, &sharedStruct) != 0) {
      fprintf(stderr, "Unable to create thread number : %d\n", i);
      exit(EXIT_FAILURE);
    }
  }

  pthread_mutex_unlock(&sharedStruct.lock_set);
  pthread_cond_wait(&sharedStruct.signalFinish, &sharedStruct.lock_signal);
  for (int i = 0; i < N_THREADS; i++) {
    pthread_cancel(tid[i]);
  }

  printf("HEIGHT OF THE TREE : %d | NB OF ELEMENT ADDED : %d\n",
         sharedStruct.set->height, cpt);

  /* Extraction du resultat , on veut : (a_even-a)(b-b_even)^(-1) (mod q)  */
  mpz_t r;
  mpz_init(r);
  mpz_sub(r, sharedStruct.collision_1.n,
          sharedStruct.collision_2.n); /* r = (b - b_even) (mod q) */
  mpz_mod(r, r, sharedStruct.q);       /* r = (b - b_even) (mod q) */

  /* test si (b - b_even) est inversible (mod q) */
  if (mpz_cmp_si(r, 0) == 0) {
    printf("ECHEC \n");
    echec = true;
    // goto end;
  }

  if (!echec) {
    mpz_invert(r, r, sharedStruct.q); /* r= (b-b_even)^(-1) (mod q) */
    mpz_sub(sharedStruct.collision_2.m, sharedStruct.collision_2.m,
            sharedStruct.collision_1.m); /* a_even = a_even-a (mod q) */
    mpz_mul(r, r, sharedStruct.collision_2.m);
    mpz_mod(r, r, sharedStruct.q);

    printf("r = ");
    PRINT2(r)

    /* liberation de la mémoire */

    mpz_clear(sharedStruct.g);
    mpz_clear(sharedStruct.h);
    mpz_clear(sharedStruct.p);
    mpz_clear(sharedStruct.q);
    deleteTree(sharedStruct.set);
    for (int i = 0; i < N; i++) {
      mpz_clear(sharedStruct.listExposant[i].m);
      mpz_clear(sharedStruct.listExposant[i].n);
      mpz_clear(sharedStruct.listM[i]);
    }
  } else {
    for (int i = 0; i < N; i++) {
      mpz_clear(sharedStruct.listExposant[i].m);
      mpz_clear(sharedStruct.listExposant[i].n);
      mpz_clear(sharedStruct.listM[i]);
    }
  }

  gettimeofday(&end, NULL);
  long int temps = ((end.tv_sec * 1000000 + end.tv_usec) -
                    (start.tv_sec * 1000000 + start.tv_usec));

  float vrai_temps = (float)temps / 1000000;
  printf("temps = %f\n", vrai_temps);
  fprintf(out, "%f\n", temps);
  //}
  fclose(in);
  fclose(out);
  return EXIT_SUCCESS;
}
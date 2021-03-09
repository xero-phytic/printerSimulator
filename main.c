#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/* cree un boolean true = 0, false = 1 */
typedef enum {
    false, true
} bool;

/*  lire  une fichier plein texte passe dans un pointeur
 *  et retourner un autre pointeur chaîne de caractère avec le contenu
 *  de fichier
 */
char *read_file(char *filename);

/* écrire une chaîne de caractères dans une pipe  */
void *pipe_write(int fd[], char *string);

/* lire une chaîne de caractères de la pipe */
void *pipe_read(int fd[]);

/* prototype conçu pour appeler avec threads
 * écrire une chaîne de caractères dans une pipe ce
 *  fait appel write_pipe en passent un file descriptor globale FD
 */
void *pipe_write_t(char *filename);

/* vérifier si c'est la dernière itération et return true or false */
bool is_last();

/* affichier une chaîne de caractères dans un rectangle */
void print_0(char *s);

/* Function supplémentaire pour prouvé que les threads ne bloque pas tous les autre procss */
void do_something();

void print_array(const int *arr, int n);
void swap(int *a, int *b);
void bubble_sort(int *arr, int size);
void gen_test_rand_arr();
/* mutex variable pour controler race condition pendant lecture de la pipe */
pthread_mutex_t mutexRead;

/*mutex variable pour controler race condition pendant lecture de la pipe */
pthread_mutex_t mutexWrite;

/* pipe file descriptor */
int FD[2];

/* variable pour tenire le nombre d'iteration  */
int iteration = 0;

/* variable pour initialiser le nombre max des itiration au nombre de fichier
 et éviter de soustraire - 1 a chaque fois */
int maxIteration = 0;


int main(int argc, char *argv[]) {

/* initialize  mutext
 print erreur message and exit if it NOT succeed */
    if (pthread_mutex_init(&mutexRead, NULL) != 0) {
        perror("ERR : init mutexRead");
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(&mutexWrite, NULL) != 0) {
        perror("ERR : init mutexWrite");
        return EXIT_FAILURE;
    }

    /* cree une pie afficher un message si erreur */
    if (pipe(FD) < 0) {
        perror("ERR : creating pipe");
        return EXIT_FAILURE;
    }

    /* variable pour initialiser le nombre max des itérations
     * au nombre du fichier et éviter de soustraire - 1 a chaque fois
     */
    maxIteration = argc - 1;

    /* var pour tenir le id processuse */
    pid_t processID;

    /* cree un tableaux de dim = maxIteration ( nombre de fichier ) de type pthread */
    pthread_t thread[maxIteration];

    /* clone process */
    if ((processID = fork()) == 0) {
        //child process
        /* si il ya pas d'argument terminer avec échec le procss child */
        if (argc == 1) {
            return EXIT_FAILURE;
        }

        /* crée des threds  maxIteration de fois  / maxIteration = nombre de fichiers passés en argument */
        /* appeler la fonction pipe_read avec un cast dans pipe_write, changer ça signature
        *  en changent sont argument en pointeur void
        * le premier void * la valeur de retour
        * (*) un pointeur vers la fonction
        * (void *) l'argument de la function
        */
        for (int i = 0; i < maxIteration; i++) {
            iteration = i + 1;
            pthread_create(thread + i, NULL, (void *(*)(void *)) pipe_write_t, argv[iteration]);
        }
        //do somthing while waitnig

        /* attendre les threads qui'l de termine avant de terminer le procss  */
        for (int i = 0; i < maxIteration; i++) {
            pthread_join(*(thread + i), NULL);
        }

    } else {
        //main process
        /* afficher un message aucun fichier n'est passé */
        if (argc == 1) {
            print_0(argv[0]);
        }

        /* crée des threds  maxIteration de fois  / maxIteration = nombre de fichiers passés en argument */
        /* appeler la fonction pipe_read avec un cast dans pipe_read, changer ça signature
        *  en changent sont argument en pointeur void
        * le premier void * la valeur de retour
        * (*) un pointeur vers la fonction
        * (void *) l'argument de la function
        */
        for (int i = 0; i < maxIteration; i++) {
            pthread_create(thread, NULL, (void *(*)(void *)) pipe_read, FD);
        }

        //do something while waiting ...
        /* décommentez la ligne  142 et 143et la sleep de la ligne N ° 257 pour mieux simuler l'imprimante
          * et vérifiez que les threads fonctionnent correctement  */
//        srand(time(NULL));
//        do_something();
        /* attendre les threads avant de terminer le processus  */
        for (int i = 0; i < maxIteration; i++) {
            pthread_join(*thread, NULL);
        }

        /*attendre le procss fille avant de terminer le procss parent*/
        wait(NULL);
    }
    /* détruir les mutex*/
    pthread_mutex_destroy(&mutexRead);
    pthread_mutex_destroy(&mutexWrite);
    return EXIT_SUCCESS;
}


void do_something() {
    printf("gen and sort array while waintig for print\n");
    int i = 0;
    while (true) {
        i++;
        printf("Time passed ... %d\n", i);
        gen_test_rand_arr(); //generer est affichier un tableux Organisé par ordre croissant
        sleep(1); // attendre
    }
}

/* functions*/
char *read_file(char *filename) {
    char *buffer = NULL;
    int strSize, read_size;
    //open file in read mode
    FILE *pIoFile = fopen(filename, "r");

    if (pIoFile) {

        /* // mettre le cursor a la fin de fichier */
        fseek(pIoFile, 0, SEEK_END);

        /*retourner la position actuel autrement dit le nombre de chars*/
        strSize = ftell(pIoFile);

        /* mettre le cursor au début*/
        rewind(pIoFile);

        /* allouer un espace pour tenir tous les caractère de fichier
        * le +1 pour ajouter le carter de fin de string
        * */
        buffer = (char *) malloc(sizeof(char) * (strSize + 1));

        /* lire tous les caractères de fichier  et stocker dans buffer */
        read_size = fread(buffer, sizeof(char), strSize, pIoFile);

        /* ajouter le carater de fin de fichier à la fin de la chaine  */
        buffer[strSize] = '\0';

        /* si l'opération n'est est terminée correctment affichier erreur libre les espace déjà réserver */
        if (strSize != read_size) {
            perror("Something went wrong while reading\n ");
            free(buffer);
            buffer = NULL;
        }
        /* fermer le fchier */
        fclose(pIoFile);
    }
    return buffer;
}

void *pipe_write(int fd[], char *string) {
    /* fermer pipe lecteur car on n'aurait pas besoin */
    close(fd[0]);
    /* calculer la taille de la chain de caractère*/
    size_t strSize = strlen(string);

    /* écrire  la taille de la chain de caractère dans la pip afficher erreur si échech */
    if (write(fd[1], &strSize, sizeof(size_t)) < 0) {
        perror("couldn't write string size to pip");
    }
    /* écrire la chain de caractère dans la pipe afficher erreur si échech */
    if (write(fd[1], string, sizeof(char) * strSize) < 0) {
        perror("couldn't write to pip");
    }
    return EXIT_SUCCESS;
}

void *pipe_read(int fd[]) {
    /* locké  le changment des veriable globale avec mutext pour eméchre la colustoin de lécteur
     * la taille de la chain de character  a cause avec d'autre thread */
    pthread_mutex_lock(&mutexRead);

    /*fermer la partie écriture pour la pipe car on n'a pas besoin*/
    close(fd[1]);

    size_t sizeString;

    /* lire la taille de la chain de caracter */
    if (read(fd[0], &sizeString, sizeof(size_t)) < 0) {
        perror("Err : reading size_t\n");
    }

    /* reserver un espace pour cette chaine de cacarcter*/
    char *string = malloc(sizeString);
    if (read(fd[0], string, sizeof(char) * sizeString) < 0) {
        perror("Err : reading string\n");
    }
    string[sizeString] = '\0';
    /* affchier la chain de cacacter */
    printf("%s\n", string);
    /* librer l'espace reszerver par malloc*/
    free(string);
    /* verfier si c'est le dernier fichier puis fermer la partie lecture de la pipe */
    if (is_last()) {
        close(fd[0]);
    }
    //time pomping ink
//    sleep(3);
    /*librer pour les autre thread */
    pthread_mutex_unlock(&mutexRead);
    return EXIT_SUCCESS;
}

void *pipe_write_t(char *filename){
    pthread_mutex_lock(&mutexWrite);
    char *string = read_file(filename);
    pipe_write(FD, string);
    pthread_mutex_unlock(&mutexWrite);
    return EXIT_SUCCESS;
}

bool is_last() {
    if (iteration == maxIteration) {
        (iteration)++;
        return true;
    }
    return false;
}

void print_0(char *s) {
    char *msg = "\n#\tNO FILE ARGUMENT PASSED ! ";
    struct winsize window;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window);
    int pound = window.ws_col / 2 ;
    int space = pound - strlen(msg);
    for (int i = 0; i < pound; i++) {
        printf("#");
    }
    printf("%s", msg);
    for (int i = 0; i < space -6;++i) {
        printf(" ");
    }
    printf("#\n");
    for (int i = 0; i < pound; i++) {
        printf("#");
    }
}

void print_array(const int *arr, int n){
    for (int i = 0; i < n; i++){
        printf("%d ", arr[i]);
    }
    printf("\n");
}
void swap(int *a, int *b){
    int temp = *a;
    *a = *b;
    *b = temp;
}

void bubble_sort(int *arr, int size){
    for (int i = 0; i < size - 1; i++){
        bool swapped = false;
        for (int j = 0; j < size - 1 - i; j++){
            if (arr[j] > arr[j + 1]){
                swap(&arr[j], &arr[j + 1]);
                swapped = true;
            }
        }
        if (!swapped){
            break;
        }
    }
}

void gen_test_rand_arr(){
    const int size = 100;
    int *arr = (int *)calloc(size, sizeof(int));
    for (int i = 0; i < size; i++){
        arr[i] = rand() % 100;
    }
    bubble_sort(arr, size);
    print_array(arr, size);
    free(arr);
}
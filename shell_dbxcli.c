#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>

long unsigned int MAX_LENGTH = 300;
long unsigned int MAX_CAPACITY = 100;
char **;

void  INThandler(int sig) {
    signal(sig, SIG_IGN);

    for (int i = 0; i < MAX_CAPACITY; i ++) {
        free(input[i]);
    }

    free(input);
    printf ("\nProgramul s-a oprit\n");
    exit(0);
}

void delete_first(char *path) {
    char aux = (char) malloc(MAX_LENGTH * sizeof(char));
    strcpy (aux, path + 1);
    strcpy(path, aux);
    free(aux);
}

void format_(char *path) {
    while (path[0] == '/' || path[0] == '.') {
        delete_first(path);
    }

    while (path[strlen(path) - 1] == '/' || path[strlen(path) - 1] == ' ' || path[strlen(path) - 1] == '\n') {
        path[strlen(path) - 1] = '\0';
    }
}

int file_or_folder_local(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);

    if (S_ISREG(path_stat.st_mode)) {
        return 0;   // File
    }
    DIR* dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 1;   // Folder
    }
    return -1;  // Non-existent file/folder
}

int file_or_folder_dbxcli(const char *path) {
    char *comanda = (char *) malloc(MAX_LENGTH * sizeof(char));
    strcpy(comanda , "./dbxcli ls ");
    strcat(comanda, path);
    strcat(comanda, " > /tmp/V.TXT 2>null");

    if (system(comanda)) {
        free(comanda);
        return -1;  // Non-existent file/folder
    }
    free(comanda);

    FILE *F = fopen("/tmp/V.TXT", "r");
    char *p = (char *) malloc (MAX_LENGTH * sizeof(char));
    getline(&p, &MAX_LENGTH, F);
    format_(p);
    fclose(F);

    if (strcmp(p, path) == 0) {
        free(p);
        return 0;   // File
    }

    free(p);
    return 1;   // Folder
}

unsigned long int find_dest_index(unsigned long int fin) {
    unsigned long int idx = fin;
    for (int i = 0; i < fin; i ++) {
        if (input[i][0] != '-') {
            idx = i;
        }
    }
    if (idx != fin) {
        format_(input[idx]);
    }
    return idx;
}

unsigned long int find_src_index(unsigned long int fin) {
    for (int i = 2; i < fin; i ++) {
        if (input[i][0] != '-') {
            format_(input[i]);
            return i;
        }
    }
    return fin;
}

int run_input(char **this_input, const char *target) {
    pid_t pid = fork();

    if (pid < 0) {
        return -1;
    } else if (pid == 0) {
        if (execv(target, this_input) < 0 ) {
            perror(NULL);
            return errno;
        }
    } else {
        wait(NULL);
        printf("\n");
        return 0;
    }
}

int set_input(char **this_input, const char *prefix) {
    char *target = (char *) malloc(MAX_CAPACITY * sizeof(char));
    strcpy(target, prefix);
    strcat(target, this_input[0]);
    run_input(this_input, target);
    free(target);
}

void is_file(char *get_put, char *path, char *dest_path) {
    char *aux = (char *) malloc (MAX_LENGTH * sizeof(char));
    strcpy(aux, get_put);
    strcat(aux, path);
    strcat(aux, " ");
    strcat(aux, dest_path);
    system(aux);
    free(aux);
}

void is_folder(char *dest_path, char *dir) {
    char *aux = (char *) malloc (MAX_LENGTH * sizeof(char));
    strcpy(aux, dir);
    strcat(aux, dest_path);
    system(aux);
    free(aux);
}

void process_folder(char *path, char *source, char *get_put, unsigned long int dest) {
    format_(path);
    format_(source);

    char *dest_path = (char *) malloc (MAX_LENGTH * sizeof(char));
    strcpy(dest_path, input[dest]);

    if (strcmp(get_put, "get") == 0) {
        strcat(dest_path, "/");
        strcat(dest_path, strstr(path, source));
        if (file_or_folder_dbxcli(path)) {
            is_folder(dest_path, "mkdir ");
        } else {
            is_file("./dbxcli get ", path, dest_path);
        }
    } else {
        strcat(dest_path, strstr(path, source) + strlen(source));
        if (file_or_folder_local(path)) {
            is_folder(dest_path, "./dbxcli mkdir ");
        } else {
            is_file("./dbxcli put ", path, dest_path);
        }
    }

    free(dest_path);
}

void write_ls(unsigned long int length, char *p) {
    for (int i = 2; i < length; i ++) {
        strcat(p, input[i]);
        strcat(p, " ");
    }
    strcat(p, " > /tmp/W.TXT");
    system(p);
}

void put_local_ls(unsigned long int dest, char *real_source) {
    FILE *F = fopen("/tmp/W.TXT", "r");
    char *path = (char *) malloc (MAX_LENGTH * sizeof(char));
    char *p = (char *) malloc (MAX_LENGTH * sizeof(char));
    char *full_path = (char *) malloc (MAX_LENGTH * sizeof(char));

    while (getline(&p, &MAX_LENGTH, F) > 0)  {
        format_(p);
        if (strlen(p) == 0) {
            continue;   // Empty line between 2 folders
        }
        if (p[strlen(p) - 1] == ':') {
            p[strlen(p) - 1] = '\0';
            strcpy(path, p);
            continue;   // Folder
        }
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, p);
        process_folder(full_path, real_source, "put", dest);
    }

    free(path);
    free(p);
    free(full_path);
    fclose(F);
}

void local_ls() {
    FILE *F = fopen("/tmp/W.TXT", "r");
    char *p = (char *) malloc (MAX_LENGTH * sizeof(char));

    while (getline(&p, &MAX_LENGTH, F) > 0) {
        printf("%s", p);
    }

    free(p);
    fclose(F);
}

void ls_or_get(char *path, char *real_source, unsigned long int dest) {
    if (path[0] != '\0') {
        if (dest == 0) {
            format_(path);
            printf("%s\n", path);
        } else {
            process_folder(path, real_source, "get", dest);
        }
    }
}

void dbxcli_ls(unsigned long int dest, char *real_source) {
    FILE *F = fopen("/tmp/W.TXT", "r");
    char *p = (char *) malloc (MAX_LENGTH * sizeof(char));
    char *buffer = (char *) malloc (MAX_LENGTH * sizeof(char));

    while (getline(&p, &MAX_LENGTH, F) > 0) {
        format_(p);
        char *aux1 = p - 1;
        while (aux1 + 1 != NULL) {
            char *aux2 = strstr(aux1 + 1, " /");
            if (aux2 == NULL) {
                ls_or_get(aux1 + 1, real_source, dest);
                break;
            }
            strncpy (buffer, aux1 + 1, aux2 - aux1 - 1);
            buffer[aux2 - aux1 - 1] = '\0';
            ls_or_get(buffer, real_source, dest);
            aux1 = aux2;
        }
        if (feof(F)) {
            break;
        }
    }

    fclose(F);
    free(p);
    free(buffer);
}

void print_help() {
    strcpy(input[0], "/dbxcli");
    strcpy(input[1], "help");
    free(input[2]);
    input[2] = NULL;
    set_input(input, ".");
    input[2] = (char *) malloc (MAX_LENGTH * sizeof(char));

    printf("Add \"l\" before any local inputs\n");
    printf("Use \"exit\" to stop the program\n\n");
}

void read_(char shell_prompt[], unsigned long int *length) {
    char *s = readline(shell_prompt);
    add_history(s);

    char *path = strtok(s, " \n");
    while (path != NULL) {
        strcpy(input[*length], path);
        (*length) ++;
        path = strtok(NULL, "  \n");
    }

    free(input[*length]);
    input[*length] = NULL;
    free(s);
}

void run_put_get(int dest_type, int src_type, unsigned long int src, unsigned long int dest, char *function) {
    if (dest_type == 0) {
        printf("Error: File already exists\n");
        return ;
    }
    if (src_type == -1) {
        printf("Error: Source doesn't exist\n");
        return ;
    }
    if (src_type == 0) {
        set_input(input, ".");  // Source is a file
        return ;
    }
    char aux = (char) malloc(MAX_LENGTH * sizeof(char));
    if (strcmp(function, "get") == 0) {
        strcpy(aux, "./dbxcli ls -R ");
    } else {
        strcpy(aux, "ls -R ");
    }
    strcat(aux, input[src]);
    strcat(aux, " > /tmp/W.TXT");
    system(aux);
    free(aux);

    // Source is a folder
    char *p = strrchr(input[src], '/');
    if (p == NULL) {
        p = input[src];
    }
    if (strcmp(function, "get") == 0) {
        dbxcli_ls(dest, p);
    } else {
        put_local_ls(dest, p);
    }
}

void run_get(unsigned long int length) {
    unsigned long int src = find_src_index(length);
    unsigned long int dest = find_dest_index(length);
    int dest_type = file_or_folder_local(input[dest]);
    int src_type = file_or_folder_dbxcli(input[src]);

    run_put_get(dest_type, src_type, src, dest, "get");
}

void format_destination(unsigned long int src, unsigned long int dest) {
    if (file_or_folder_dbxcli(input[dest]) == 1) {
        char *p = strrchr(input[src], '/');
        if (p == NULL) {
            strcat(input[dest], "/");
            strcat(input[dest], input[src]);
        } else {
            strcat(input[dest], p);
        }
    }
}

void run_put(unsigned long int length) {
    unsigned long int src = find_src_index(length);
    unsigned long int dest = find_dest_index(length);
    format_destination(src, dest);
    int dest_type = file_or_folder_dbxcli(input[dest]);
    int src_type = file_or_folder_local(input[src]);

    run_put_get(dest_type, src_type, src, dest, "put");
}

int main() {
    input = (char *) malloc (MAX_CAPACITY * sizeof(char));
    for (int i = 0; i < MAX_CAPACITY; i ++) {
        input[i] = (char *) malloc(MAX_LENGTH * sizeof(char));
    }
    signal(SIGINT, INThandler);

    print_help();
    char shell_prompt[MAX_CAPACITY];
    snprintf(shell_prompt, MAX_LENGTH, "%s:%s $ ", getenv("USER"), getcwd(NULL, 1024));

    while(1) {
        unsigned long int length = 1;
        read_(shell_prompt, &length);

        if (strcmp(input[1], "exit") == 0) {
            break;
        }

        if (strcmp(input[1], "ls") == 0) {
            char p = (char) malloc(MAX_LENGTH * sizeof(char));
            strcpy(p, "./dbxcli ls ");
            write_ls(length, p);
            dbxcli_ls(0, NULL);
            free(p);
        } else if (strcmp(input[1], "lls") == 0) {
            char p = (char) malloc(MAX_LENGTH * sizeof(char));
            strcpy(p, "ls ");
            write_ls(length, p);
            local_ls();
            free(p);
        } else if (input[1][0] == 'l' && strcmp(input[1], "logout") != 0) {
            delete_first(input[1]);
            set_input(input + 1, "/bin/");
        } else if(strcmp(input[1], "get") == 0) {
            run_get(length);
        } else if(strcmp(input[1], "put") == 0) {
            run_put(length);
        } else {
            set_input(input, ".");
        }
        input[length] = (char *) malloc (MAX_LENGTH * sizeof(char));
    }

    for (int i = 0; i < MAX_CAPACITY; i ++) {
        free(input[i]);
    }
    free(input);
    return 0;
}

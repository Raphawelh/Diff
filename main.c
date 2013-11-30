#include "lcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "edit_list.h"
#include "line_length_list.h"

#define ARGV_INPUT true

char** load_file(char* filename, int* nb_lines);
int get_number_of_lines(FILE* file);
Line_Length* get_lines_data(char* text, int* nb);
void print_diff(char** file_a, int size_a, char** file_b, int size_b, char** lcs, int size_lcs);

int main(int argc, char** argv)
{

#ifdef ARGV_INPUT
    if (argc < 3) {
        puts("Nombre de parametres insuffisants");
        return 1;
    }
    char* path_a = argv[1];
    char* path_b = argv[2];
#else
    char path_a[256] = {};
    char path_b[256] = {};
    fgets(path_a, 256, stdin);
    fgets(path_b, 256, stdin);
    path_a[strlen(path_a)-1] = '\0';
    path_b[strlen(path_b)-1] = '\0';
#endif

    int size_a = 0;
    int size_b = 0;

    char** file_a = load_file(path_a, &size_a);
    if (file_a == NULL)
        return 0;

    char** file_b = load_file(path_b, &size_b);
    if (file_b == NULL)
        return 0;

    char** lcs = calloc(sizeof(char*), max(size_a, size_b) + 1);

    char** matrix = build_lcs_matrix(file_a, file_b, size_a, size_b);
    extract_lcs(matrix, file_a, file_b, size_a, size_b, lcs);

    putchar('\n');

    int size_lcs = 0;
    while (lcs[size_lcs++] != 0);
    size_lcs--;

    print_diff(file_a, size_a, file_b, size_b, lcs, size_lcs);

    return 0;
}

int get_nb_lines_until_found(char** file, char* string, int start, int size) {

    int i;
    for (i = 0 ; i < size && strcmp(file[i + start], string) != 0 ; i++ );

    return i;

}
void print_lines(char** file, int start, int count, char symbol) {

    int i;
    for (i = 0 ; i < count ; i++ )
        printf("%c %s", symbol, file[start + i]);
}


#define print_range(start, count)                  \
    if (count == 1)                                \
        printf("%d", start+1);                     \
    else                                           \
        printf("%d,%d", start + 1, start + count); \

    // Rappel: Si un caractère est dans le lcs, alors il est obligatoirement présent dans la chaine de départ ET d'arrivée
    /*  Pour chaque élément du lcs et dans l'ordre
            Si on ne trouve pas cet élément lcs dans B mais en le trouvant dans A, c'est que des lignes ont été ajoutées avant dans B
                On indique toutes ces lignes comme des ajouts et on les passe
            Si on ne trouve pas cet élément lcs dans A mais en le trouvant dans B, c'est que des lignes ont été supprimées dans B
                On indique toutes ces lignes comme des suppressions et on les passe
            Si on ne trouve cet élément lcs ni dans A ni dans B, c'est que ces lignes ont été changées de A en celles de B
                On indique toutes ces lignes comme changées et on les passe
    */
void print_diff(char** file_a, int size_a, char** file_b, int size_b, char** lcs, int size_lcs) {

    bool found_in_a = true;
    bool found_in_b = true;
    int i, idx_a = 0, idx_b = 0;
    int nb_lines_different_a;
    int nb_lines_different_b;

    for (i = 0 ; i < size_lcs ; i++ )
    {
        nb_lines_different_a = 0;
        nb_lines_different_b = 0;

        found_in_a = !strcmp(lcs[i], file_a[idx_a]);

        found_in_b = !strcmp(lcs[i], file_b[idx_b]);

        // Trouvé dans A mais pas au meme endroit dans B => additions dans B
        if (found_in_a && !found_in_b)
        {
            nb_lines_different_b = get_nb_lines_until_found(file_b, lcs[i], idx_b, size_b);

            printf("%d", idx_a);
            putchar('a');
            print_range(idx_b, nb_lines_different_b);

            putchar('\n');

            print_lines(file_b, idx_b, nb_lines_different_b, '>');
        }
        // Trouvé dans B mais pas au meme endroit dans A => suppressions dans A
        if (!found_in_a && found_in_b)
        {
            nb_lines_different_a = get_nb_lines_until_found(file_a, lcs[i], idx_a, size_a);

            print_range(idx_a, nb_lines_different_a);
            putchar('d');
            printf("%d", idx_b);

            putchar('\n');

            print_lines(file_a, idx_a, nb_lines_different_a, '<');
        }
        if (!found_in_a && !found_in_b)
        {
            nb_lines_different_a = get_nb_lines_until_found(file_a, lcs[i], idx_a, size_a);
            nb_lines_different_b = get_nb_lines_until_found(file_b, lcs[i], idx_b, size_b);

            print_range(idx_a, nb_lines_different_a);
            putchar('c');
            print_range(idx_b, nb_lines_different_b);

            putchar('\n');

            print_lines(file_a, idx_a, nb_lines_different_a, '<');
            puts("-------");
            print_lines(file_b, idx_b, nb_lines_different_b, '>');
        }

        idx_b += nb_lines_different_b;
        idx_a += nb_lines_different_a;
        idx_a++;
        idx_b++;
    }

}

FILE* open_file(char* filename) {

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Impossible d'ouvrir '%s'\n", filename);
    }

    return file;
}

char** load_file(char* filename, int* nb_lines) {

    FILE* file = open_file(filename);
    if (file == NULL)
        return NULL;

    puts("---------------------");

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);

    char* buffer = malloc(sizeof(char) * size);

    fread(buffer, sizeof(char), size, file);
    rewind(file);

    Line_Length* liste = get_lines_data(buffer, nb_lines);
    printf("nb_lines: %d\n", *nb_lines);

    Line_Length* curseur = liste;

    char** text = malloc(sizeof(char*) * *nb_lines);
    int i;
    for (i = 0 ; i < *nb_lines ; i++ )
    {
        text[i] = calloc(sizeof(char), curseur->length + 1);
        strncpy(text[i], curseur->data, curseur->length);
        printf("%s", text[i]);
        curseur = curseur->next;
    }

    return text;

}

int get_number_of_lines(FILE* file) {

    if (file == NULL)
        return 0;

    int nb_lines = 0;
    int i = 0;
    char c;

    while ( (c = fgetc(file)) != EOF && c != '\0')
    {
        if (c == '\n')
            nb_lines++;
    }

    return nb_lines;
}


Line_Length* get_lines_data(char* text, int* nb) {

    if (text == NULL)
        return 0;

    Line_Length* liste = NULL;

    int nb_lines = 0;
    int length = 0;
    int i = 0;
    char* data = text;

    while (text[i] != EOF && text[i] != '\0')
    {
        length++;
        if (text[i] == '\n')
        {
            (*nb)++;
            liste = empilerFin_LL(liste, length, data);
            length = 0;
            data = text+i+1;
        }
//        nb += (text[i] == '\n');
        i++;
    }

    return liste;
}












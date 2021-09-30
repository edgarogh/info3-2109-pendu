#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#ifdef NDEBUG
#include <time.h>
#endif

// Nombre maximal d'erreurs avant l'échec
const u_int64_t MAX_ERRORS = 10;

typedef struct DictionaryEntry_ {
    struct DictionaryEntry_* next;
    char* word;
} DictionaryEntry;

/**
 * Stocke un dictionnaire sous la forme d'une liste chaînée de longueur `len`.
 */
typedef struct {
    DictionaryEntry* head;
    size_t len;
} Dictionary;

char* Dictionary_get(Dictionary self, size_t index) {
    assert(index < self.len);

    DictionaryEntry* entry = self.head;
    while (0 != index--) {
        assert(entry != NULL);
        entry = entry->next;
    }

    return entry->word;
}

char* Dictionary_get_random_word(Dictionary self) {
    return Dictionary_get(self, rand() % self.len);
}

size_t read_catch(ssize_t result) {
    if (result == -1) {
        fprintf(stderr, "read() a rencontré une erreur: %s\n", strerror(errno));
        exit(1);
    } else {
        return result;
    }
}

/**
 * @warning Not thread-safe
 * @returns Malloc-ed strings
 */
char* read_line(int fd) {
    static char BUFFER[256];

    size_t index = 0;
    while (read_catch(read(fd, &BUFFER[index], 1)) == 1) {
        if (BUFFER[index] == '\n') {
            BUFFER[index] = 0;
            char* word = malloc(index + 1);
            strcpy(word, BUFFER);
            return word;
        }

        if (++index >= sizeof(BUFFER)) {
            fprintf(stderr, "Ligne trop longue pour être lue\n");
            exit(1);
        }
    }

    return NULL;
}

Dictionary Dictionary_read(char* file_name) {
    int file = open(file_name, O_RDONLY);
    if (file == -1) {
        fprintf(stderr, "Le fichier \"%s\" n'a pas pu être ouvert: %s\n", file_name, strerror(errno));
        exit(1);
    }

    Dictionary dictionary = {
            .len = 0,
    };

    DictionaryEntry** tail = &dictionary.head;

    char* line;
    while ((line = read_line(file))) {
        if (line[0] == 0 || line[0] == '#') {
            // Empty lines and "comments" are skipped
            continue;
        }

        DictionaryEntry* current = malloc(sizeof(DictionaryEntry));
        current->word = line;
        *tail = current;
        tail = &current->next;

        dictionary.len ++;
    }

    // "Close" the linked list
    *tail = NULL;

    close(file);

    return dictionary;
}

/**
 * État d'une partie en cours. `word` est le mot secret à deviner et `revealed` est une liste `malloc`ée des lettres
 * révélées. Le type de pointeur de `word` n'est pas spécifié.
 *
 * Cette structure doit être construite avec le constructeur `State_new()`
 */
typedef struct {
    char* word;
    bool* revealed;
} State;

State State_new(char* word) {
    return (State) {
        .word = word,
        .revealed = calloc(strlen(word), sizeof(bool)),
    };
}

/**
 * Essaye de révéler la lettre `c` et renvoie `true` si elle était présente dans le mot
 */
bool State_reveal(State* self, char c) {
    bool success = false;

    char current;
    for (size_t i = 0; (current = self->word[i]) != 0; i++) {
        if (current == c) {
            self->revealed[i] = true;
            success = true;
        }
    }

    return success;
}

void State_print(State* self) {
    char current;
    for (size_t i = 0; (current = self->word[i]) != 0; i++) {
        printf("%c", self->revealed[i] ? current : '-');
    }
    printf("\n");
}

bool State_is_fully_revealed(State* self) {
    for (size_t i = 0; self->word[i] != 0; i++) {
        if (!self->revealed[i]) return false;
    }

    return true;
}

char read_char() {
    char ch;

    do {
        if (read(STDIN_FILENO, &ch, 1) == -1) {
            fprintf(stderr, "Impossible de lire depuis l'entrée standard. Arrêt.\n");
            exit(1);
        }
    } while (ch == '\n');

    return ch;
}

/**
 * Vide l'entrée standard pour ne pas laisser un `\n` dans le buffer du shell.
 *
 * Honteusement volé ici: https://stackoverflow.com/a/26081123 @ M.M.
 *   J'imagine que je pourrais utiliser `read` comme au dessus à la place de `getchar` mais j'aime bien l'aspect
 *   "one-liner" de cette solution.
 */
void empty_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
#ifdef NDEBUG
    // En mode "release", on seede le RNG, mais pas en "debug"
    srand(clock());
#else
    printf("\e[33m⚠ Mode debug: le RNG n'est pas seedé !\n"
           "Compilez avec `-DNDEBUG` pour avoir la version \"release\".\n\e[0m");
#endif

    char* secret = Dictionary_get_random_word(Dictionary_read("dico.txt"));

    State state = State_new(secret);

    printf("Vous avez %ld coups pour deviner le mot :\n", MAX_ERRORS);
    State_print(&state);

    for (size_t attempt = 0; attempt < MAX_ERRORS;) {
        printf("? "); fflush(stdout);
        char c = read_char();
        if (!State_reveal(&state, c)) attempt++;
        State_print(&state);

        if (State_is_fully_revealed(&state)) {
            empty_stdin();
            printf("Bravo ! Le mot était \"%s\" et a été trouvé avec %ld erreur·s.\n", state.word, attempt);
            return 0;
        }
    }

    empty_stdin();
    printf("Malheureusement, vous avez échoué.\n");

    return 0;
}

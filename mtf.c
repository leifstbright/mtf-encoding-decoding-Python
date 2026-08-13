/* Leif St Bright */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 200
#define MAX_WORD_LEN 64
#define NEW_WORD_MARKER 0xff

int count = 1;
char *place_list[MAX_WORDS];
int place_count = 0;

/* dictionary entry: tracks each word's current MTF rank */
typedef struct {
    char word[MAX_WORD_LEN];
    int rank;
} DictEntry;

DictEntry word_dict[MAX_WORDS];
int dict_size = 0;

// header bytes written at the start of every .mtf file
unsigned char magic_numbers[4] = {0xba, 0x5e, 0xba, 0x11};

/* adjusts the order of the words in the list */
void adjust(const char *w) {
    int i, j, idx = -1;

    for (i = 0; i < place_count; i++) {
        if (strcmp(place_list[i], w) == 0) {
            idx = i;
            break;
        }
    }

    if (idx >= 0) {
        char *tmp = place_list[idx];
        for (j = idx; j > 0; j--) {
            place_list[j] = place_list[j - 1];
        }
        place_list[0] = tmp;
    } else {
        for (j = place_count; j > 0; j--) {
            place_list[j] = place_list[j - 1];
        }
        place_list[0] = strdup(w);
        place_count++;
    }
}

/* finds the word in the list and reorganizes the list */
char *find_and_decode(int pla) {
    int index = pla - 128;  /* positions start at 0 */
    char *w = place_list[index];
    int j;

    for (j = index; j > 0; j--) {
        place_list[j] = place_list[j - 1];
    }
    place_list[0] = w;

    return w;
}

/* gets the word from the dictionary and reorganizes it.
   returns the word's rank if it already exists, or -1 if it's new */
int encode(const char *w) {
    int i, o;

    if (dict_size == 0) {
        strcpy(word_dict[dict_size].word, w);
        word_dict[dict_size].rank = 0;
        dict_size++;
        count++;
        return -1;
    }

    for (i = 0; i < dict_size; i++) {
        if (strcmp(word_dict[i].word, w) == 0) {
            o = word_dict[i].rank;
            for (i = 0; i < dict_size; i++) {
                if (strcmp(word_dict[i].word, w) != 0 && word_dict[i].rank < o) {
                    word_dict[i].rank += 1;
                }
            }
            for (i = 0; i < dict_size; i++) {
                if (strcmp(word_dict[i].word, w) == 0) {
                    word_dict[i].rank = 0;
                    break;
                }
            }
            return o;
        }
    }

    /* word not in dict: increment existing ranks, then insert new at 0 */
    for (i = 0; i < dict_size; i++) {
        word_dict[i].rank += 1;
    }
    strcpy(word_dict[dict_size].word, w);
    word_dict[dict_size].rank = 0;
    dict_size++;
    count++;
    return -1;
}

// reads a text file word by word and writes the encoded .mtf output
void encode_main(const char *file) {
    char output[256];
    char base_name[256];
    char *dot, *slash;
    FILE *binary_file, *f;
    char line[1024];

    // strip directory and extension to build the output filename
    strcpy(base_name, file);
    slash = strrchr(base_name, '/');
    if (slash) memmove(base_name, slash + 1, strlen(slash + 1) + 1);
    dot = strrchr(base_name, '.');
    if (dot) *dot = '\0';
    sprintf(output, "%s.mtf", base_name);

    binary_file = fopen(output, "wb");
    if (!binary_file) { perror("fopen output"); exit(1); }
    fwrite(magic_numbers, 1, 4, binary_file);

    f = fopen(file, "r");
    if (!f) { perror("fopen input"); exit(1); }

    while (fgets(line, sizeof(line), f)) {
        char *word = strtok(line, " \t\r\n");
        while (word != NULL) {
            int rank = encode(word);
            if (rank >= 0) {
                unsigned char output_write = (unsigned char)(rank + 128);
                fwrite(&output_write, 1, 1, binary_file);
            } else {
                unsigned char count_write = (unsigned char)(count + 127);
                fwrite(&count_write, 1, 1, binary_file);
                fwrite(word, 1, strlen(word), binary_file);
            }
            word = strtok(NULL, " \t\r\n");
        }
        fputc(0x0a, binary_file);
    }

    fclose(f);
    fclose(binary_file);
}

// reads the .mtf file back into place_list order and writes plain text
void decode_main(const char *bin_file) {
    char output[256];
    char base_name[256];
    char *dot, *slash;
    FILE *b, *t;
    unsigned char *data;
    long n, i;
    char word[MAX_WORD_LEN];
    int word_len = 0;
    char *line_words[1024];
    int line_word_count = 0;
    int place = -1;
    unsigned char prev_char = 0;
    unsigned char current;

    strcpy(base_name, bin_file);
    slash = strrchr(base_name, '/');
    if (slash) memmove(base_name, slash + 1, strlen(slash + 1) + 1);
    dot = strrchr(base_name, '.');
    if (dot) *dot = '\0';
    sprintf(output, "%s.txt", base_name);
    printf("writing to %s\n", output);

    // load the whole file into memory so escape/marker bytes can be
    // looked at without worrying about line-based reads splitting them
    b = fopen(bin_file, "rb");
    if (!b) { perror("fopen input"); exit(1); }
    fseek(b, 0, SEEK_END);
    n = ftell(b);
    fseek(b, 0, SEEK_SET);
    data = malloc(n);
    fread(data, 1, n, b);
    fclose(b);

    t = fopen(output, "w");
    if (!t) { perror("fopen output"); exit(1); }
    printf("Writing to %s\n", output);

    i = 4;  /* skip the 4-byte magic header (only appears at the start) */
    while (i < n) {
        current = data[i];

        if (current == 0x0a) {
            if (word_len > 0) {
                word[word_len] = '\0';
                line_words[line_word_count++] = strdup(word);
                adjust(word);
                word_len = 0;
            }
            if (line_word_count > 0) {
                int k;
                for (k = 0; k < line_word_count; k++) {
                    fputs(line_words[k], t);
                    if (k < line_word_count - 1) fputc(' ', t);
                    free(line_words[k]);
                }
                fputc('\n', t);
            } else if (prev_char == 0x0a) {
                fputc('\n', t);
            }
            line_word_count = 0;
            place = -1;
            i++;

        } else if (current < 0x80) {
            word[word_len++] = (char)current;
            i++;

        } else {
            if (word_len > 0) {
                word[word_len] = '\0';
                line_words[line_word_count++] = strdup(word);
                adjust(word);
                word_len = 0;
            }
            if (current < 128 + place_count) {
                line_words[line_word_count++] = strdup(find_and_decode(current));
                place = -1;
            } else {
                place = current;
            }
            i++;
        }
        prev_char = current;
    }

    if (word_len > 0) {
        word[word_len] = '\0';
        line_words[line_word_count++] = strdup(word);
        adjust(word);
    }
    if (line_word_count > 0) {
        int k;
        for (k = 0; k < line_word_count; k++) {
            fputs(line_words[k], t);
            if (k < line_word_count - 1) fputc(' ', t);
            free(line_words[k]);
        }
        fputc('\n', t);
    }

    fclose(t);
    free(data);
}

// picks encode or decode based on the file extension
int main(int argc, char *argv[]) {
    char *file;
    size_t len;

    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    file = argv[1];
    len = strlen(file);

    if (len > 4 && strcmp(file + len - 4, ".mtf") == 0) {
        decode_main(file);
    } else {
        encode_main(file);
    }

    return 0;
}

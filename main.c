#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

#define HASH_TABLE_SIZE 101
#define MAX_TITLE_LEN 100
#define MAX_AUTHOR_LEN 100
#define MAX_GENRE_LEN 50
#define FILENAME "livros.txt"

typedef struct Book {
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    struct Book* next;
} Book;

typedef struct GenreEntry {
    char genre[MAX_GENRE_LEN];
    Book* books_head;
    struct GenreEntry* next;
} GenreEntry;

GenreEntry* hash_table[HASH_TABLE_SIZE];


unsigned int hash(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_TABLE_SIZE;
}

Book* create_book(const char* title, const char* author) {
    Book* new_book = malloc(sizeof(Book));
    if (!new_book) {
        fprintf(stderr, "Erro na alocação de memória para livro\n");
        exit(EXIT_FAILURE);
    }
    strncpy(new_book->title, title, MAX_TITLE_LEN);
    new_book->title[MAX_TITLE_LEN - 1] = '\0';
    strncpy(new_book->author, author, MAX_AUTHOR_LEN);
    new_book->author[MAX_AUTHOR_LEN - 1] = '\0';
    new_book->next = NULL;
    return new_book;
}

GenreEntry* create_genre_entry(const char* genre) {
    GenreEntry* new_entry = malloc(sizeof(GenreEntry));
    if (!new_entry) {
        fprintf(stderr, "Erro na alocação de memória para gênero\n");
        exit(EXIT_FAILURE);
    }
    strncpy(new_entry->genre, genre, MAX_GENRE_LEN);
    new_entry->genre[MAX_GENRE_LEN - 1] = '\0';
    new_entry->books_head = NULL;
    new_entry->next = NULL;
    return new_entry;
}

GenreEntry* find_genre_entry(const char* genre) {
    unsigned int index = hash(genre);
    GenreEntry* current = hash_table[index];
    while (current) {
        if (strcmp(current->genre, genre) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

GenreEntry* add_genre(const char* genre) {
    unsigned int index = hash(genre);
    GenreEntry* genre_entry = find_genre_entry(genre);
    if (genre_entry)
        return genre_entry;
    GenreEntry* new_entry = create_genre_entry(genre);
    new_entry->next = hash_table[index];
    hash_table[index] = new_entry;
    return new_entry;
}

void add_book_to_genre(const char* genre, const char* title, const char* author) {
    GenreEntry* genre_entry = find_genre_entry(genre);
    if (!genre_entry)
        genre_entry = add_genre(genre);
    Book* new_book = create_book(title, author);
    new_book->next = genre_entry->books_head;
    genre_entry->books_head = new_book;
    printf("✅ Livro '%s' de %s adicionado ao gênero '%s'.\n", title, author, genre);
}


void save_to_file(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Erro ao abrir arquivo para salvamento");
        return;
    }

    // Escreve o BOM UTF-8 para garantir acentuação correta
    fprintf(fp, "\xEF\xBB\xBF");

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        GenreEntry* genre = hash_table[i];
        while (genre) {
            fprintf(fp, "GENRE:%s\n", genre->genre);
            Book* book = genre->books_head;
            while (book) {
                fprintf(fp, "TITLE:%s\n", book->title);
                fprintf(fp, "AUTHOR:%s\n", book->author);
                book = book->next;
            }
            fprintf(fp, "ENDGENRE\n");
            genre = genre->next;
        }
    }

    fclose(fp);
    printf("💾 Dados salvos em '%s'.\n", filename);
}

void load_from_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("🔍 Nenhum arquivo encontrado, iniciando biblioteca vazia.\n");
        return;
    }

    char line[256];
    char current_genre[MAX_GENRE_LEN] = "";

    
    if (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "\xEF\xBB\xBF", 3) != 0) {
            fseek(fp, 0, SEEK_SET);
        }
    }

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0; 

        if (strncmp(line, "GENRE:", 6) == 0) {
            strncpy(current_genre, line + 6, MAX_GENRE_LEN);
            current_genre[MAX_GENRE_LEN - 1] = '\0';
        }
        else if (strncmp(line, "TITLE:", 6) == 0) {
            char title[MAX_TITLE_LEN];
            char author[MAX_AUTHOR_LEN];

            strncpy(title, line + 6, MAX_TITLE_LEN);
            title[MAX_TITLE_LEN - 1] = '\0';

            if (fgets(line, sizeof(line), fp) && strncmp(line, "AUTHOR:", 7) == 0) {
                line[strcspn(line, "\n")] = 0;
                strncpy(author, line + 7, MAX_AUTHOR_LEN);
                author[MAX_AUTHOR_LEN - 1] = '\0';
                add_book_to_genre(current_genre, title, author);
            }
        }
        else if (strcmp(line, "ENDGENRE") == 0) {
            strcpy(current_genre, "");
        }
    }

    fclose(fp);
    printf("📥 Dados carregados de '%s'.\n", filename);
}


void recommend_books(const char* genre) {
    GenreEntry* genre_entry = find_genre_entry(genre);
    if (!genre_entry || !genre_entry->books_head) {
        printf("⚠ Nenhum livro encontrado para o gênero '%s'.\n", genre);
        return;
    }
    printf("\n🎯 Recomendações para '%s':\n", genre);
    Book* current = genre_entry->books_head;
    int count = 1;
    while (current) {
        printf("%d. %s, autor: %s\n", count++, current->title, current->author);
        current = current->next;
    }
}

void search_book_by_title(const char* title) {
    int found = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        GenreEntry* genre = hash_table[i];
        while (genre) {
            Book* book = genre->books_head;
            while (book) {
                if (strcasecmp(book->title, title) == 0) {
                    printf("✅ Livro encontrado: %s (Autor: %s, Gênero: %s)\n", book->title, book->author, genre->genre);
                    found = 1;
                }
                book = book->next;
            }
            genre = genre->next;
        }
    }
    if (!found) {
        printf("⚠ Livro '%s' não encontrado.\n", title);
    }
}

void remove_book(const char* genre, const char* title) {
    GenreEntry* genre_entry = find_genre_entry(genre);
    if (!genre_entry) {
        printf("⚠ Gênero '%s' não encontrado.\n", genre);
        return;
    }

    Book* current = genre_entry->books_head;
    Book* prev = NULL;

    while (current) {
        if (strcasecmp(current->title, title) == 0) {
            if (prev) prev->next = current->next;
            else genre_entry->books_head = current->next;
            free(current);
            printf("✅ Livro '%s' removido do gênero '%s'.\n", title, genre);
            return;
        }
        prev = current;
        current = current->next;
    }

    printf("⚠ Livro '%s' não encontrado no gênero '%s'.\n", title, genre);
}

void list_all_books() {
    int found = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        GenreEntry* genre = hash_table[i];
        while (genre) {
            printf("\n📚 Gênero: %s\n", genre->genre);
            Book* book = genre->books_head;
            if (!book) {
                printf("   Nenhum livro.\n");
            }
            int count = 1;
            while (book) {
                printf("   %d. %s (Autor: %s)\n", count++, book->title, book->author);
                book = book->next;
                found = 1;
            }
            genre = genre->next;
        }
    }
    if (!found) {
        printf("⚠ Nenhum livro cadastrado.\n");
    }
}


void free_books(Book* head) {
    while (head) {
        Book* tmp = head;
        head = head->next;
        free(tmp);
    }
}

void free_hash_table() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        GenreEntry* current = hash_table[i];
        while (current) {
            GenreEntry* tmp = current;
            current = current->next;
            free_books(tmp->books_head);
            free(tmp);
        }
        hash_table[i] = NULL;
    }
}


void read_line(char* buffer, int size) {
    if (fgets(buffer, size, stdin)) {
        buffer[strcspn(buffer, "\n")] = 0;
    }
}

void clear_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


void menu() {
    int choice;
    char genre[MAX_GENRE_LEN];
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];

    while (1) {
        printf("\n=== 📚 Sistema de Recomendação de Livros ===\n");
        printf("1. ➕ Cadastrar livro\n");
        printf("2. 🎯 Recomendar livros por gênero\n");
        printf("3. 🔍 Buscar livro por título\n");
        printf("4. 🗑 Remover livro\n");
        printf("5. 📜 Listar todos os livros\n");
        printf("6. 💾 Salvar e sair\n");
        printf("Escolha uma opção: ");

        if (scanf("%d", &choice) != 1) {
            clear_stdin();
            printf("⚠ Entrada inválida. Tente novamente.\n");
            continue;
        }
        clear_stdin();

        switch (choice) {
            case 1:
                printf("Informe o gênero: ");
                read_line(genre, MAX_GENRE_LEN);

                printf("Informe o título do livro: ");
                read_line(title, MAX_TITLE_LEN);

                printf("Informe o autor do livro: ");
                read_line(author, MAX_AUTHOR_LEN);

                add_book_to_genre(genre, title, author);
                break;
            case 2:
                printf("Informe o gênero: ");
                read_line(genre, MAX_GENRE_LEN);
                recommend_books(genre);
                break;
            case 3:
                printf("Informe o título: ");
                read_line(title, MAX_TITLE_LEN);
                search_book_by_title(title);
                break;
            case 4:
                printf("Informe o gênero: ");
                read_line(genre, MAX_GENRE_LEN);
                printf("Informe o título: ");
                read_line(title, MAX_TITLE_LEN);
                remove_book(genre, title);
                break;
            case 5:
                list_all_books();
                break;
            case 6:
                save_to_file(FILENAME);
                printf("Saindo...\n");
                return;
            default:
                printf("⚠ Opção inválida. Tente novamente.\n");
        }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    for (int i = 0; i < HASH_TABLE_SIZE; i++)
        hash_table[i] = NULL;

    load_from_file(FILENAME);
    menu();
    free_hash_table();

    return 0;
}

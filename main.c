#include "particle_simulation.h"

int main(void) {
    String file_data = str_from_cstr("");
    String path = str_from_cstr("./main.c");
    read_entire_file(path, &file_data);
    printf("%s", file_data.items);
    str_free(&file_data);
    str_free(&path);
    return 0;
}

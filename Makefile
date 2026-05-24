# tarsau - Sistem Programlama 2025-2026 Bahar Donemi Projesi
#
# Kaynagi src/ altindan derleyip proje kokune `tarsau` isminde calisabilir
# bir dosya birakir. Object dosyalari src/ icinde tutulur, boylelikle kaynak
# agaci nesneleri kaynaktan ayri tutar.

CC          := cc
CFLAGS      := -Wall -Wextra -Werror -Wpedantic -std=c11 -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS     :=

BINARY      := tarsau
SRC_DIR     := src

SOURCES     := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/sau_args.c \
	$(SRC_DIR)/sau_ascii_guard.c \
	$(SRC_DIR)/sau_blueprint.c \
	$(SRC_DIR)/sau_error.c \
	$(SRC_DIR)/sau_fs_ops.c \
	$(SRC_DIR)/sau_pack_writer.c \
	$(SRC_DIR)/sau_path_safety.c \
	$(SRC_DIR)/sau_unpack_reader.c

OBJECTS     := $(SOURCES:.c=.o)

.PHONY: all clean

all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(BINARY)
	rm -f *.sau *.sau.tmp.*

# Пути и переменные
CC = gcc
CFLAGS = -Wall -Wextra -g -I../libmysyslog
LDFLAGS = 
SRC_DIR = src
OBJ_DIR = obj
BIN = myRPC-server
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
LIB = ../libmysyslog/libmysyslog.a

# Цель по умолчанию
all: $(BIN)

# Сборка исполняемого файла
$(BIN): $(OBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Сборка объектных файлов
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Создание директории для объектов
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Очистка
clean:
	rm -rf $(BIN) $(OBJ_DIR) *.deb DEBIAN control.tar.gz data.tar.gz myrpc-server_*.deb

# Сборка статической библиотеки
$(LIB):
	$(MAKE) -C ../libmysyslog

# Сборка deb-пакета
deb: all
	mkdir -p deb_build/DEBIAN
	mkdir -p deb_build/usr/sbin
	cp $(BIN) deb_build/usr/sbin/
	echo "Package: myrpc-server" > deb_build/DEBIAN/control
	echo "Version: 1.0" >> deb_build/DEBIAN/control
	echo "Section: base" >> deb_build/DEBIAN/control
	echo "Priority: optional" >> deb_build/DEBIAN/control
	echo "Architecture: amd64" >> deb_build/DEBIAN/control
	echo "Maintainer: Your Name <you@example.com>" >> deb_build/DEBIAN/control
	echo "Description: myRPC server for remote command execution" >> deb_build/DEBIAN/control
	dpkg-deb --build deb_build myrpc-server_1.0_amd64.deb

.PHONY: all clean deb

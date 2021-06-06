CC = /usr/bin/gcc
CFLAGS = -Wall -Werror -g
LDFLAGS = -lcrypto -lssl

all: build

build: client.h server.h
#	$(CC) $(CFLAGS) -o openssl main.c client.c server.c $(LDFLAGS)
	$(CC) $(CFLAGS) -o ssl-test main.c client.c server.c openssl_hostname_validation.c $(LDFLAGS)

clean:
#	rm -f *.o core openssl
	rm -f *.o core ssl-test

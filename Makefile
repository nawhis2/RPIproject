# Top-level Makefile for project (builds both server and client)
all: server client

server:
	$(MAKE) -C server
	$(MAKE) -C server send

client:
	$(MAKE) -C client

clean:
	$(MAKE) -C server clean
	$(MAKE) -C client clean

fclean: clean
	rm -f server/server client/client

re: fclean all

.PHONY: all server client clean fclean re
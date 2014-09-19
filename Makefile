CC=mpicc
CFLAGS="-Wall"

debug:clean
	$(CC) $(CFLAGS) -g -o bigmatrixmpi main.c mpiutil.c normals.c
stable:clean
	$(CC) $(CFLAGS) -o bigmatrixmpi main.c mpiutil.c normals.c
test:clean
	$(CC) $(CFLAGS) -g -o bigmatrixmpiTest test.c mpiutil.c normals.c
clean:
	rm -vfr *~ bigmatrixmpi

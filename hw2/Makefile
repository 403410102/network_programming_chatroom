include ../Make.defines

PROGS =	cli serv
	

all:	${PROGS}



serv:	serv.o 
		${CC} ${CFLAGS} -o $@ serv.o ${LIBS}

cli:	cli.o
		${CC} ${CFLAGS} -o $@ cli.o ${LIBS}


clean:
		rm -f ${PROGS} ${CLEANFILES}

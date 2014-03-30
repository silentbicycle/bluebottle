PROJECT = bluebottle
OPTIMIZE = -O3
WARN = -Wall -pedantic -Wextra
CFLAGS += -std=c99 -g ${WARN} ${OPTIMIZE}

all: test_${PROJECT}

OBJS= bluebottle.o

TEST_OBJS= 

test_${PROJECT}: test_${PROJECT}.o ${OBJS} ${TEST_OBJS}
	${CC} -o $@ test_${PROJECT}.o ${OBJS} ${TEST_OBJS} ${LDFLAGS}

test: ${PROJECT}
	./test_${PROJECT}

clean:
	rm -f ${PROJECT} test_${PROJECT} *.o *.core

ifdef CROSS_COMPILE
	compile_prefix := ${CROSS_COMPILE}
else
	compile_prefix := 
endif

all: 
	${compile_prefix}gcc -o writer writer.c

clean:
	rm -rf writer

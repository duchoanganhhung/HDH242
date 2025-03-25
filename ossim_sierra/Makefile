INC = -Iinclude
LIB = -lpthread

SRC = src
OBJ = obj
INCLUDE = include

CC = gcc
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG) $(INC)
LFLAGS = -Wall $(DEBUG) $(INC)

vpath %.c $(SRC)
vpath %.h $(INCLUDE)

# Object files needed by modules
MEM_OBJ = $(addprefix $(OBJ)/, paging.o mem.o cpu.o loader.o)
SYSCALL_OBJ = $(addprefix $(OBJ)/, syscall.o sys_killall.o sys_mem.o sys_listsyscall.o)
OS_OBJ = $(addprefix $(OBJ)/, cpu.o mem.o loader.o queue.o os.o sched.o timer.o mm-vm.o mm.o mm-memphy.o libstd.o libmem.o)
OS_OBJ += $(SYSCALL_OBJ)
SCHED_OBJ = $(addprefix $(OBJ)/, cpu.o loader.o)
HEADER = $(wildcard $(INCLUDE)/*.h)
 
all: os

# Compile the syscall table
syscalltbl.lst: $(SRC)/syscall.tbl
	@echo "Generating syscall table..."
	chmod +x $(SRC)/syscalltbl.sh
	$(SRC)/syscalltbl.sh $< $(SRC)/syscalltbl.lst 

# Compile OS with all syscalls
os: $(OBJ) syscalltbl.lst $(OS_OBJ)
	$(CC) $(LFLAGS) $(OS_OBJ) -o os $(LIB)

# Compile syscall objects
$(OBJ)/%.o: %.c ${HEADER} $(OBJ)
	$(CC) $(CFLAGS) $< -o $@

# Prepare objectives container
$(OBJ):
	mkdir -p $(OBJ)

clean:
	rm -f $(SRC)/*.lst
	rm -f $(OBJ)/*.o os sched mem
	rm -rf $(OBJ)

test_syscall: $(OBJ) syscalltbl.lst $(SYSCALL_OBJ) obj/mem.o obj/mm.o obj/mm-vm.o obj/mm-memphy.o obj/libmem.o src/test.c
	$(CC) $(LFLAGS) $(SYSCALL_OBJ) obj/mem.o obj/mm.o obj/mm-vm.o obj/mm-memphy.o obj/libmem.o src/test.c -o test


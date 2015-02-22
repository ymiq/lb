
TARGET := bin/structs

SRCS += test/main.cpp
SRCS += test/lbdb.cpp
SRCS += src/rcu_man.cpp
SRCS += src/lb_table.cpp
SRCS += src/stat_obj.cpp
SRCS += src/stat_table.cpp
SRCS += src/stat_man.cpp

CPPFLAGS = -O2 -Werror -I./include
LDFLAGS = -l stdc++ -l pthread -L/usr/lib64/mysql/ -lmysqlclient

.PHONY: default clean
default: clean
	gcc $(CPPFLAGS) $(LDFLAGS) $(SRCS) -o $(TARGET)

test: test_clean
	gcc $(CFLAGS) $(LDFLAGS) test/test.cpp -o bin/test

test_clean:
	rm -fr bin/test

clean:
	rm -f $(TARGET)


TARGET := structs

SRCS = main.cpp
SRCS += rcu_man.cpp
SRCS += lb_table.cpp
SRCS += stat_table.cpp

CPPFLAGS = -O2 -Werror 
LDFLAGS = -l stdc++ -l pthread

.PHONY: default clean
default: clean
	gcc $(CPPFLAGS) $(LDFLAGS) $(SRCS) -o $(TARGET)

test: test_clean
	gcc $(CFLAGS) $(LDFLAGS) test.cpp -o test

test_clean:
	rm -fr test.o test

clean:
	rm -f $(TARGET) *.o 

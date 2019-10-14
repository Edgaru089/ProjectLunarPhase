CPPFLAGS=-std=c++17 $(EXTRA_CPPFLAGS)
LDLIBS=-lsfml-network -lsfml-system -lpthread -lmysqlclient $(EXTRA_LDLIBS)
EXTRA_LDLIBS=

all: Instance.o HTTPResponseWrapper.o Main.o MainF.o MySql/MySql.o MySql/MySqlException.o MySql/MySqlPreparedStatement.o MySql/OutputBinder.o SFNUL/HTTP.o
	g++ *.o MySql/*.o SFNUL/*.o -o ../RunDir/LunarPhase $(LDLIBS)
clean:
	rm -f *.o MySql/*.o SFNUL/*.o ../RunDir/LunarPhase
.PHONY: all clean


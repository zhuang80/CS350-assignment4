R=readwrite

all:$(R);

clean:
	rm -rf $(R).o
	rm -rf *.txt
	rm -rf $(R)

$(R).o:$(R).cpp
	g++ -c -lpthread $(R).cpp -o $(R).o

$(R):$(R).o
	g++ -lpthread $(R).o -o $(R)
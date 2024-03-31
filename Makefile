

all: gendlopen

clean:
	cd test && $(MAKE) clean
	cd src && $(MAKE) clean
	cd examples && $(MAKE) clean

gendlopen:
	cd src && $(MAKE)

check: gendlopen
	cd test && $(MAKE)

examples: gendlopen
	cd examples && $(MAKE)


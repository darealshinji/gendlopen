# NMake Makefile

bin  = src\gendlopen.exe
make = nmake -nologo


all: $(bin)

clean:
	cd src && $(make) clean
	cd examples && $(make) clean

$(bin):
	cd src && $(make)

test: $(bin)
	cd examples && $(make)

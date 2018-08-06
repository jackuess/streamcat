piratepersist: piratepersist.c output.c
	gcc -pedantic -std=c99 -O3 -fstrict-aliasing -Werror -Wextra -Wall -Wconversion -Wno-sign-conversion -Wstrict-aliasing -I/usr/include/curl -lcurl output.c piratepersist.c -opiratepersist

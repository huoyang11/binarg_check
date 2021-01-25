all:
	cd core && make
	cd lbcheck && gcc -g -o binarg_check.o -c binarg_check.c -I../core
	gcc -g -o main main.c lbcheck/binarg_check.o obj/ngx_palloc.o obj/ngx_array.o obj/ngx_string.o -Ilbcheck -Icore

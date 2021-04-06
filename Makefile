CFLAGS = /MD /c
build: main list hashMap
	link main.obj list.obj hashMap.obj /out:so-cpp.exe	
list:
	cl $(CFLAGS) list.c
hashMap:
	cl $(CFLAGS) hashMap.c
main:
	cl $(CFLAGS) main.c
clean:
	del main.obj List.obj HashMap.obj so-cpp.exe

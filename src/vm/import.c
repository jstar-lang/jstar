#include "import.h"
#include "memory.h"
#include "compiler.h"
#include "hashtable.h"
#include "modules.h"

#include "parse/parser.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

static char *loadSource(const char *name) {
	size_t nameLength = strlen(name);
	char *fname = malloc(nameLength + 4); // +4 for NUL and `.bl`
	strcpy(fname, name);
	strcat(fname + nameLength, ".bl");

	FILE *srcFile = fopen(fname, "r+");
	if(srcFile == NULL || errno == EISDIR) {
		if(srcFile) fclose(srcFile);
		free(fname);
		return NULL;
	}

	fseek(srcFile, 0, SEEK_END);
	size_t size = ftell(srcFile);
	rewind(srcFile);

	char *src = malloc(size + 1);
	if(src == NULL) {
		fclose(srcFile);
		free(fname);
		return NULL;
	}

	size_t read = fread(src, sizeof(char), size, srcFile);
	if(read < size) {
		free(src);
		free(fname);
		fclose(srcFile);
		return NULL;
	}

	free(fname);
	fclose(srcFile);

	src[read] = '\0';
	return src;
}

ObjFunction *compileWithModule(VM *vm, ObjString *name, Stmt *program) {
	ObjModule *module = getModule(vm, name);

	if(module == NULL) {
		disableGC(vm, true);

		module = newModule(vm, name);

		ObjModule *core = getModule(vm, copyString(vm, "__core__", 8));
		if(core != NULL) {
			hashTableMerge(&module->globals, &core->globals);
		}

		hashTablePut(&module->globals, copyString(vm, "__name__", 8), OBJ_VAL(name));

		setModule(vm, name, module);

		disableGC(vm, false);
	}

	ObjFunction *fn = compile(vm, module, program);

	return fn;
}

void setModule(VM *vm, ObjString *name, ObjModule *module) {
	hashTablePut(&vm->modules, name, OBJ_VAL(module));
}

ObjModule *getModule(VM *vm, ObjString *name) {
	Value module;
	if(!hashTableGet(&vm->modules, name, &module)) {
		return NULL;
	}
	return AS_MODULE(module);
}

bool importModule(VM *vm, ObjString *name) {
	if(hashTableContainsKey(&vm->modules, name)) {
		push(vm, NULL_VAL);
		return true;
	}

	bool dyn = true;
	const char *src = NULL;
	if((src = readBuiltInModule(name->data)) != NULL) {
		dyn = false;
	} else {
		src = loadSource(name->data);
	}

	if(src == NULL) return false;

	Parser p;
	Stmt *program = parse(&p, src);

	if(p.hadError) {
		freeStmt(program);
		if(dyn) free((char*)src);
		return false;
	}

	ObjFunction *fn = compileWithModule(vm, name, program);

	if(dyn) free((char*)src);
	freeStmt(program);

	if(fn == NULL) return false;

	push(vm, OBJ_VAL(fn));
	return true;
}

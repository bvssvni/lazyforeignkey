#if 0
#!/bin/bash
clear
gcc -o bin/test-lazyforeignkey -g test-lazyforeignkey.c lazyforeignkey.c -O3 -Wall -Wfatal-errors
if [ "$?" = "0" ]; then
	time ./bin/test-lazyforeignkey
fi
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lazyforeignkey.h"

//////////////// Prints lot of text if memory leak //////////////
// http://isprogrammingeasy.blogspot.no/2013/04/detecting-memory-leaks-in-c.html
#define free(ptr) \
do { \
static long min = -1; \
static long max = -1; \
long ptr_val = (long)ptr; \
long diff = max - min; \
min = min == -1 || ptr_val < min ? ptr_val : min; \
max = max == -1 || ptr_val > max ? ptr_val : max; \
if (max - min > diff) { \
	printf("%ld %s FILE %s LINE %i\n", (max - min), #ptr, __FILE__, __LINE__); \
} \
\
free(ptr); \
} while (0);
/////////////////////////////////////////////////////////////////

typedef struct {
	LFK_Table *table;
} People;

typedef struct {
	long id;
} Person;

Person InsertPerson(People people, int val) {
	LFK_ForeignKey key = LFK_AddRow(people.table);
	((int*)people.table->data[0])[key.llup] = val;
	return (Person){key.id};
}

void DeleteRow(LFK_Table *table, int pos) {
	table->ids[pos] = -1;
}

LFK_ForeignKey LFK_CreateForeignKey(long id) {
	return (LFK_ForeignKey){.id = id, .llup = -1};
}

void TestDefragmentation(void) {
	People people = (People){LFK_NewTable(100)};
	LFK_AddColumn(people.table, sizeof(int));
	
	int i;
	int cl = 1 << 0;
	for (i = 0; i < cl; i++) {
	
		Person carl = InsertPerson(people, 3);
		LFK_ForeignKey carl_key = LFK_CreateForeignKey(carl.id);
		LFK_LookUp(people.table, &carl_key);
		
		assert(carl_key.llup == 0);
		DeleteRow(people.table, 0);
		LFK_LookUp(people.table, &carl_key);
		assert(carl_key.id == -1L);
		
		Person john = InsertPerson(people, 5);
		Person sara = InsertPerson(people, 7);
		
		LFK_ForeignKey john_key = LFK_CreateForeignKey(john.id);
		LFK_LookUp(people.table, &john_key);
		assert(john_key.llup == 1);
		LFK_ForeignKey sara_key = LFK_CreateForeignKey(sara.id);
		LFK_LookUp(people.table, &sara_key);
		assert(sara_key.llup == 2);
		
		LFK_Defragment(people.table);
		
		LFK_LookUp(people.table, &john_key);
		assert(john_key.llup == 0);
		LFK_LookUp(people.table, &sara_key);
		assert(sara_key.llup == 1);
		
		DeleteRow(people.table, 0);
		DeleteRow(people.table, 1);
		
		LFK_Defragment(people.table);
	}
	
	LFK_FreeTable(people.table);
}

typedef struct {
	LFK_Table *table;
} Vectors;

typedef struct {
	int id;
} Vector;

Vector InsertVector(Vectors v, float x, float y) {
	LFK_ForeignKey key = LFK_AddRow(v.table);
	((float*)v.table->data[0])[key.llup] = x;
	((float*)v.table->data[1])[key.llup] = y;
	return (Vector){key.id};
}

float FloatValue(LFK_Table *table, int col, int row) {
	return ((float*)table->data[col])[row];
}

void TestReadWrite(void) {
	Vectors t = (Vectors){LFK_NewTable(4)};
	LFK_AddColumn(t.table, sizeof(float));
	LFK_AddColumn(t.table, sizeof(float));
	InsertVector(t, 10, 20);
	InsertVector(t, 11, 21);
	
	const char* file = "testfiles/test.bin";
	FILE *f = fopen(file, "w");
	LFK_Write(t.table, f);
	fclose(f);
	
	LFK_FreeTable(t.table);
	
	f = fopen(file, "r");
	Vectors t2 = (Vectors){LFK_Read(f)};
	fclose(f);
	
	assert(t2.table->column_len == 2);
	assert(t2.table != NULL);
	assert(FloatValue(t2.table, 0, 0) == 10);
	assert(FloatValue(t2.table, 1, 0) == 20);
	assert(FloatValue(t2.table, 0, 1) == 11);
	assert(FloatValue(t2.table, 1, 1) == 21);

	LFK_FreeTable(t2.table);
}

typedef struct {
	LFK_Table *table;
} Strings;

typedef struct {
	LFK_ForeignKey key;
} String;

Strings NewStringsTable(int capacity) {
	LFK_Table *table = LFK_NewTable(capacity);
	LFK_AddColumn(table, sizeof(char*));
	return (Strings){table};
}

void FreeStringsTable(Strings *strings) {
	if (strings->table == NULL) return;
	
	int i;
	for (i = 0; i < strings->table->len; i++) {
		char *text = ((char**)strings->table->data[0])[i];
		free(text);
	}
	
	LFK_FreeTable(strings->table);
	strings->table = NULL;
}

const char *GetString(Strings strings, String string) {
	LFK_LookUp(strings.table, &string.key);
	const char *text = ((char**)strings.table->data[0])[string.key.llup];
	return text;
}

char *CopyString(int n, char *text) {
	char *copy = malloc(sizeof(char) * (n + 1));
	memcpy(copy, text, sizeof(char) * n);
	copy[n] = '\0';
	return copy;
}

void StringToFile(FILE *f, const char *text) {
	int n = strlen(text);
	fwrite(&n, sizeof(n), 1, f);
	fwrite(text, sizeof(*text), n, f);
}

char *StringFromFile(FILE *f) {
	char *copy = NULL;

	int n;
	int bytesRead = fread(&n, sizeof(n), 1, f);
	if (bytesRead == 0) goto ERROR;

	copy = malloc(sizeof(char) * (n+1));
	bytesRead = fread(copy, sizeof(*copy), n, f);
	if (bytesRead == 0) goto ERROR;

	copy[n] = '\0';
	return copy;

ERROR:
	if (copy != NULL) free(copy);

	return NULL;
}

String InsertString(Strings strings, char *text) {
	char *copy = CopyString(strlen(text), text);
	LFK_ForeignKey key = LFK_AddRow(strings.table);
	((char**)strings.table->data[0])[key.llup] = copy;
	return (String){key};
}

void WriteStrings(Strings strings, FILE *f) {
	// Make sure data is packed.
	LFK_Defragment(strings.table);
	// Save number of rows.
	int rows = strings.table->len;
	fwrite(&rows, sizeof(rows), 1, f);

	int i;
	for (i = 0; i < strings.table->len; i++) {
		char *text = ((char**)strings.table->data[0])[i];
		StringToFile(f, text);
	}
}

Strings ReadStrings(FILE *f) {
	Strings strings = {};

	int rows;
	size_t bytes = fread(&rows, sizeof(rows), 1, f);
	if (bytes == 0) goto ERROR;

	strings = NewStringsTable(rows);
	int i;
	for (i = 0; i < rows; i++) {
		char *text = StringFromFile(f);
		if (text == NULL) goto ERROR;

		InsertString(strings, text);
		free(text);
	}
	
	return strings;

ERROR:
	if (strings.table != NULL) LFK_FreeTable(strings.table);

	return (Strings){};
}

void TestCustomReadWrite(void) {
	Strings strings = NewStringsTable(3);
	
	String carl = InsertString(strings, "Carl");
	String john = InsertString(strings, "John");
	String peter = InsertString(strings, "Peter");
	
	const char* c = GetString(strings, carl);
	const char* j = GetString(strings, john);
	const char* p = GetString(strings, peter);
	
	assert(strcmp(c, "Carl") == 0);
	assert(strcmp(j, "John") == 0);
	assert(strcmp(p, "Peter") == 0);
	
	// Save data.
	const char* file = "testfiles/test-strings.bin";
	FILE *f = fopen(file, "w");
	WriteStrings(strings, f);
	fclose(f);
	// Release memory.
	FreeStringsTable(&strings);
	
	f = fopen(file, "r");
	strings = ReadStrings(f);
	fclose(f);
	
	assert(strings.table->len == 3);
	assert(strings.table->cap == 3);
	assert(strings.table->column_len == 1);
	
	c = GetString(strings, carl);
	j = GetString(strings, john);
	p = GetString(strings, peter);
	
	assert(strcmp(c, "Carl") == 0);
	assert(strcmp(j, "John") == 0);
	assert(strcmp(p, "Peter") == 0);
	
	FreeStringsTable(&strings);
	assert(strings.table == NULL);
}

void TestExpandTable(void) {
	LFK_Table *table = LFK_NewTable(0);
	assert(table->len == 0);
	assert(table->cap == 0);
	LFK_AddRow(table);
	assert(table->len == 1);
	assert(table->cap == 1);
	LFK_AddRow(table);
	assert(table->len == 2);
	assert(table->cap == 2);
	LFK_AddRow(table);
	assert(table->len == 3);
	assert(table->cap == 4);
	LFK_FreeTable(table);
}

void TestListAdd(void) {
	LFK_List list = LFK_NewList(3);
	LFK_ForeignKey a = {.id = 1, .llup = 0};
	LFK_ForeignKey b = {.id = 2, .llup = 1};
	LFK_ForeignKey c = {.id = 3, .llup = 2};
	LFK_AddToList(&list, a);
	LFK_AddToList(&list, b);
	LFK_AddToList(&list, c);
	assert(list.len == 3);
	LFK_FreeList(&list);
}

void TestListInsertAndRemove(void) {
	LFK_List list = LFK_NewList(3);
	LFK_ForeignKey a = {.id = 1, .llup = 0};
	LFK_ForeignKey b = {.id = 2, .llup = 1};
	LFK_ForeignKey c = {.id = 3, .llup = 2};
	LFK_InsertInList(&list, 0, a);
	LFK_InsertInList(&list, 0, b);
	LFK_InsertInList(&list, 0, c);
	assert(list.len == 3);
	LFK_RemoveFromListByIndex(&list, 0);
	LFK_RemoveFromListByIndex(&list, 0);
	LFK_RemoveFromListByIndex(&list, 0);
	assert(list.len == 0);	
	LFK_FreeList(&list);
}

int main(int argc, char *argv[]) {
	int i;

	for (i = 0; i < 1; i++) TestDefragmentation();
	for (i = 0; i < 1; i++) TestReadWrite();
	for (i = 0; i < 1; i++) TestExpandTable();
	for (i = 0; i < 1; i++) TestCustomReadWrite();
	for (i = 0; i < 1; i++) TestListAdd();
	for (i = 0; i < 1; i++) TestListInsertAndRemove();
	
	return 0;
}


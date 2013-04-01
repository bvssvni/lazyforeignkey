#if 0
#!/bin/bash
clear
gcc -o bin/test-lazyforeignkey test-lazyforeignkey.c lazyforeignkey.c -O3 -Wall -Wfatal-errors
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
	int n;
	fread(&n, sizeof(n), 1, f);
	char *copy = malloc(sizeof(char) * (n+1));
	fread(copy, sizeof(*copy), n, f);
	copy[n] = '\0';
	return copy;
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
	int rows;
	fread(&rows, sizeof(rows), 1, f);
	Strings strings = NewStringsTable(rows);
	int i;
	for (i = 0; i < rows; i++) {
		char *text = StringFromFile(f);
		((char**)strings.table->data[0])[i] = text;
	}
	
	return strings;
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
	LFK_FreeTable(strings.table);
	f = fopen(file, "r");
	strings = ReadStrings(f);
	fclose(f);
	c = GetString(strings, carl);
	j = GetString(strings, john);
	p = GetString(strings, peter);
	
	assert(strcmp(c, "Carl") == 0);
	assert(strcmp(j, "John") == 0);
	assert(strcmp(p, "Peter") == 0);
	
	LFK_FreeTable(strings.table);
}

int main(int argc, char *argv[]) {
	int i;
	for (i = 0; i < 1; i++) TestDefragmentation();
	for (i = 0; i < 1; i++) TestReadWrite();
	for (i = 0; i < 1; i++) TestCustomReadWrite();
	
	return 0;
}


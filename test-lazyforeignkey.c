#if 0
#!/bin/bash
clear
gcc -o test-lazyforeignkey test-lazyforeignkey.c lazyforeignkey.c -O3 -Wall -Wfatal-errors
if [ "$?" = "0" ]; then
	time ./test-lazyforeignkey
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
	
	const char* file = "test.bin";
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

int main(int argc, char *argv[]) {
	int i;
	for (i = 0; i < 1; i++) {
		TestDefragmentation();
	}
	for (i = 0; i < 1; i++) {
		TestReadWrite();
	}
	
	return 0;
}


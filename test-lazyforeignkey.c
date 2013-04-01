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

long InsertRow(LFK_Table *table, int val) {
	long id = table->id_counter;
	int pos = table->len;
	table->ids[pos] = id;
	((int*)table->data[0])[pos] = val;
	table->id_counter++;
	table->len++;
	return id;
}

void DeleteRow(LFK_Table *table, int pos) {
	table->ids[pos] = -1;
}

LFK_ForeignKey LFK_CreateForeignKey(long id) {
	return (LFK_ForeignKey){.id = id, .llup = -1};
}

void TestDefragmentation(void) {
	LFK_Table *people = LFK_NewTable(100);
	LFK_AddColumn(people, sizeof(int));
	
	int i;
	int cl = 1 << 0;
	for (i = 0; i < cl; i++) {
	
		long carl = InsertRow(people, 3);
		LFK_ForeignKey carl_key = LFK_CreateForeignKey(carl);
		LFK_LookUp(people, &carl_key);
		
		assert(carl_key.llup == 0);
		DeleteRow(people, 0);
		LFK_LookUp(people, &carl_key);
		assert(carl_key.id == -1L);
		
		long john = InsertRow(people, 5);
		long sara = InsertRow(people, 7);
		
		LFK_ForeignKey john_key = LFK_CreateForeignKey(john);
		LFK_LookUp(people, &john_key);
		assert(john_key.llup == 1);
		LFK_ForeignKey sara_key = LFK_CreateForeignKey(sara);
		LFK_LookUp(people, &sara_key);
		assert(sara_key.llup == 2);
		
		LFK_Defragment(people);
		
		LFK_LookUp(people, &john_key);
		assert(john_key.llup == 0);
		LFK_LookUp(people, &sara_key);
		assert(sara_key.llup == 1);
		
		DeleteRow(people, 0);
		DeleteRow(people, 1);
		
		LFK_Defragment(people);
	}
	
	LFK_FreeTable(people);
}

void TestReadWrite(void) {
	LFK_Table *const t = LFK_NewTable(4);
	FILE *f = fopen("test.bin", "w");
	LFK_Write(t, f);
	fclose(f);
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


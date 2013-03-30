
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lazyforeignkey.h"

void LFK_LookUp(LFK_Table const *table, LFK_ForeignKey *key) {
	int llup = key->llup == LLUP_FIRST_TIME ? table->len - 1 : key->llup;
	int i;
	for (i = llup; i >= 0; i--) {
		long table_id = table->ids[i];
		if (table_id == -1) {
			continue;
		} else if (table_id == key->id) {
			key->llup = i;
			return;
		} else if (table_id < key->id) {
			key->llup = LLUP_DELETED;
			return;
		}
	}
	
	key->llup = LLUP_DELETED;
	return;
}

int LFK_NewColumn(LFK_Table *table, int item_size) {
	int id = table->column_len;
	table->column_len++;
	table->column_size = realloc(table->column_size,
								 sizeof(*table->column_size) * table->column_len);
	table->data = realloc(table->data,
							 sizeof(*table->data) * table->column_len);
	table->data[id] = malloc(item_size * table->len);
	table->column_size[id] = item_size;
	memset(table->data[id], 0, item_size * table->len);
	return id;
}

int LFK_NewForeignKey(LFK_Table *table) {
	int id = table->foreignkey_columns;
	table->foreignkey_columns++;
	table->foreignkey_values = realloc
	(table->foreignkey_values,
	 sizeof(*table->foreignkey_values) * table->foreignkey_columns);
	table->foreignkey_values[id] = malloc(sizeof(double) * table->cap);
	memset(table->foreignkey_values[id], 0, sizeof(double) * table->cap);
	return id;
}


LFK_Table *LFK_NewTable(int capacity) {
	LFK_Table *table = malloc(sizeof(LFK_Table));
	*table = (LFK_Table){};
	table->id_counter = 1L;
	table->cap = capacity;
	table->ids = malloc(sizeof(*table->ids) * capacity);
	return table;
}

void LFK_Defragment(LFK_Table *table) {
	int i;
	int move = 0;
	for (i = 0; i < table->len; i++) {
		if (table->ids[i] == -1) {
			move++;
		} else if (move == 0) {
			continue;
		} else {
			table->ids[i - move] = table->ids[i];
			int j;
			for (j = 0; j < table->column_len; j++) {
				void *ptr = table->data[j];
				int size = table->column_size[j];
				memcpy(((unsigned char*)ptr) + (i - move) * size,
					   ((unsigned char*)ptr) + i * size,
					   size);
			}
			for (j = 0; j < table->foreignkey_columns; j++) {
				table->foreignkey_values[j][i - move] = table->foreignkey_values[j][i];
			}
			
			table->ids[i] = -1;
		}
	}
	
	table->len -= move;
}

void LFK_FreeTable(LFK_Table *table) {
	int i;
	for (i = 0; i < table->column_len; i++) {
		free(table->data[i]);
	}
	for (i = 0; i < table->foreignkey_columns; i++) {
		free(table->foreignkey_values[i]);
	}
	
	free(table->column_size);
	free(table->data);
	free(table->foreignkey_values);
	free(table->ids);
	
	free(table);
}


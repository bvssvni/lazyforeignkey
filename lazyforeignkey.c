
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lazyforeignkey.h"

LFK_ForeignKey LFK_AddRow(LFK_Table *table) {
	if (table->len == table->cap) {
		int newCap = table->cap == 0 ? 1 : table->cap << 1;
		int i;
		table->ids = realloc
		(table->ids, sizeof(*table->ids) * newCap);
		for (i = 0; i < table->column_len; i++) {
			table->data[i] = realloc
			(table->data[i], table->column_size[i] * newCap);
		}
		for (i = 0; i < table->foreignkey_len; i++) {
			table->foreignkey_values[i] = realloc
			(table->foreignkey_values[i], sizeof(LFK_ForeignKey) * newCap);
		}
		table->cap = newCap;
	}
	
	long id = table->id_counter;
	int pos = table->len;
	table->ids[pos] = id;
	table->id_counter++;
	table->len++;
	return (LFK_ForeignKey){.id = id, .llup = pos};
}

void LFK_LookUp(const LFK_Table *table, LFK_ForeignKey *key) {
	if (key->id == -1L) return;
	
	int llup = key->llup == -1 ? table->len - 1 : key->llup;
	int i;
	for (i = llup; i >= 0; i--) {
		long table_id = table->ids[i];
		if (table_id == -1L) {
			continue;
		} else if (table_id == key->id) {
			key->llup = i;
			return;
		} else if (table_id < key->id) {
			key->id = -1L;
			return;
		}
	}
	
	key->id = -1;
	return;
}

int LFK_AddColumn(LFK_Table *table, int item_size) {
	int id = table->column_len;
	table->column_len++;
	table->column_size = realloc
	(table->column_size, sizeof(*table->column_size) * table->column_len);
	table->data = realloc
	(table->data, sizeof(*table->data) * table->column_len);
	table->data[id] = malloc(item_size * table->cap);
	table->column_size[id] = item_size;
	memset(table->data[id], 0, item_size * table->cap);
	return id;
}

int LFK_AddForeignKey(LFK_Table *table) {
	int id = table->foreignkey_len;
	table->foreignkey_len++;
	table->foreignkey_values = realloc
	(table->foreignkey_values,
	 sizeof(*table->foreignkey_values) * table->foreignkey_len);
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
			for (j = 0; j < table->foreignkey_len; j++) {
				LFK_ForeignKey *foreignkeyColumn = table->foreignkey_values[j];
				foreignkeyColumn[i - move] = foreignkeyColumn[i];
			}
			
			table->ids[i] = -1;
		}
	}
	
	table->len -= move;
}

void LFK_Write(const LFK_Table *table, FILE *f) {
	fwrite(&table->len, sizeof(table->len), 1, f);
	fwrite(&table->cap, sizeof(table->cap), 1, f);
	fwrite(&table->id_counter, sizeof(table->id_counter), 1, f);
	fwrite(table->ids, sizeof(*table->ids), table->len, f);
	fwrite(&table->column_len, sizeof(table->column_len), 1, f);
	fwrite(table->column_size,
		   sizeof(*table->column_size), table->column_len, f);
	int i;
	for (i = 0; i < table->column_len; i++) {
		fwrite(table->data[i], table->column_size[i], table->len, f);
	}
	
	fwrite(&table->foreignkey_len, sizeof(table->foreignkey_len), 1, f);
	for (i = 0; i < table->foreignkey_len; i++) {
		fwrite(table->foreignkey_values[i],
			   sizeof(LFK_ForeignKey), table->len, f);
	}
}

LFK_Table *LFK_Read(FILE *f) {
	LFK_Table *table = NULL;

	int len, cap;
	size_t bytesRead = fread(&len, sizeof(len), 1, f);
	if (bytesRead == 0) goto ERROR;

	bytesRead = fread(&cap, sizeof(cap), 1, f);
	if (bytesRead == 0) goto ERROR;

	table = LFK_NewTable(cap);
	table->len = len;
	bytesRead = fread(&table->id_counter, sizeof(table->id_counter), 1, f);
	if (bytesRead == 0) goto ERROR;

	bytesRead = fread(table->ids, sizeof(*table->ids), table->len, f);
	if (bytesRead == 0) goto ERROR;

	bytesRead = fread(&table->column_len, sizeof(table->column_len), 1, f);
	if (bytesRead == 0) goto ERROR;

	// Allocate array that tells size of columns.
	if (table->column_len > 0) {
		table->column_size = malloc
		(sizeof(*table->column_size) * table->column_len);
	}
		
	bytesRead = fread(table->column_size,
		  sizeof(*table->column_size), table->column_len, f);
	if (bytesRead == 0) goto ERROR;

	// Allocate array that contains data.
	if (table->cap > 0) {
		table->data = malloc(sizeof(*table->data) * table->cap);
	}
		
	int i;
	for (i = 0; i < table->column_len; i++) {
		// Allocate data for column.
		if (table->cap > 0) {
			table->data[i] = malloc(table->column_size[i] * table->cap);
		}
		
		bytesRead = fread(table->data[i], table->column_size[i], table->len, f);
		if (bytesRead == 0) goto ERROR;	
	}
	
	bytesRead = fread(&table->foreignkey_len, sizeof(table->foreignkey_len), 1, f);
	if (bytesRead == 0) goto ERROR;

	// Allocate array that contains foreign keys.
	if (table->foreignkey_len > 0) {
		table->foreignkey_values = malloc
		(sizeof(*table->foreignkey_values) * table->foreignkey_len);
	}
		
	for (i = 0; i < table->foreignkey_len; i++) {
		bytesRead = fread(table->foreignkey_values[i],
			  sizeof(LFK_ForeignKey), table->len, f);
		if (bytesRead == 0) goto ERROR;
	}
	
	return table;

ERROR:
	if (table != NULL) LFK_FreeTable(table);

	return NULL;
}

void free_pointer(void **ptr) {
	if (*ptr == NULL) return;
	
	free(*ptr);
	*ptr = NULL;
}

void LFK_FreeTable(LFK_Table *const table) {
	int i;
	for (i = 0; i < table->column_len; i++) {
		free_pointer(&table->data[i]);
	}

	for (i = 0; i < table->foreignkey_len; i++) {
		free_pointer((void**)&table->foreignkey_values[i]);
	}

	free_pointer((void**)&table->column_size);
	free_pointer((void**)&table->data);
	free_pointer((void**)&table->foreignkey_values);
	free_pointer((void**)&table->ids);
	free_pointer((void**)&table);
}


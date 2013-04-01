/*
 lazyforeignkey - A general data table container in C.
 BSD license.
 by Sven Nilsen, 2013
 http://www.cutoutpro.com
 
 Version: 0.000 in angular degrees version notation
 http://isprogrammingeasy.blogspot.no/2012/08/angular-degrees-versioning-notation.html
 */
/*
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 */

#ifndef MEMGUARD_LAZYFOREIGNKEY
#define MEMGUARD_LAZYFOREIGNKEY

/*
 
 ABBRIVIATIONS:
 
 llup	= last look up position
 LSK	= LazyForeignKey
 len	= length
 cap	= capacity
 id		= identifier
 
 */

typedef struct LFK_ForeignKey {
	long id;	// -1 when deleted.
	int llup;	// -1 before looking up first time.
} LFK_ForeignKey;

typedef struct LFK_Table {
	int len;	// number of rows in table.
	int cap;	// row capacity.
	long id_counter;	// the next id.
	long *ids;			// the ids per row.
	int column_len;		// number of columns.
	int *column_size;	// size of each column in bytes.
	void **data;		// a pointer for each allocated column.
	int foreignkey_len;					// number of foreign keys.
	LFK_ForeignKey **foreignkey_values;	// foreign key data.
} LFK_Table;

LFK_Table *LFK_NewTable(int capacity);
LFK_ForeignKey LFK_AddRow(LFK_Table *table);
void LFK_LookUp(LFK_Table const *table, LFK_ForeignKey *key);
int LFK_AddColumn(LFK_Table *table, int item_size);
int LFK_AddForeignKey(LFK_Table *table);
void LFK_Defragment(LFK_Table *table);
void LFK_Write(const LFK_Table *table, FILE *f);
LFK_Table *LFK_Read(FILE *f);
void LFK_FreeTable(LFK_Table *table);

#endif


lazyforeignkey
==============

A general data table container in C.  
BSD license.  
For version log, view the individual files.  

#What is LazyForeignKey?

LazyForeignKey is a low-level structure for organizing data.  
The basic structure is a Table.  
A Table can contain rows with column types of any type.  

The data is maintained as raw memory.  
Because this is very unsafe, it is recommended to create functions  
on top of LazyForeignKey that inserts, updates and deletes data.  

A Table can contain foreign keys, which points to items in another or the same Table.  
When you delete a row, the foreign keys in the other tables are not updated.  
Instead, they contain a position where they last looked up position (LLUP) of the id.  
Each time a foreign key is used to look up data, it updates its LLUP.  
If it can not find the correct id, it searches backward until it reaches the correct one.  
If it finds a smaller id the row has been deleted.  
This makes look ups very fast in cases where data does not change O(1).  

WARNING: You should only add data at the end of the table.  
If you do not do this the foreign key lookup will fail.  


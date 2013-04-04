lazyforeignkey
==============

A general data table container in C.  
BSD license.  
For version log, view the individual files.  

#LazyForeignKey

1. Assumes data is always added to end of table.  
2. Foreign keys are maintained through own struct.  
3. Faster than binary search by looking up id.  
4. Searches bacwkards if the id does not match with row.  

LazyForeignKey's only purpose is to look up data by id very fast.  
It allows data to be deleted without need to cascade the change in id.  

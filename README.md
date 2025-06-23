# Disekt

A disk-sector visualisation tool
Takes a C64 image file and displays the BAM (Block-Availability Map)
in a circular disk.

It also works for checking the integrity of a disk backed up using my
janky custom tool. This means it can show the quality of a sector's
information based on whether or not the checksums match and if
a certain sector is missing or not.

## To-Do List:

 [ ] Show an overall disk health & usage indicator on the main interface (top-right)
 [ ] Include previous/next postions in analysis info based on directory information
 [ ] Change directory listing to only show the files in the current directory block
 [ ] Add analysis info to the directory pages (Show how many blocks in each file and how many are healthy)
 [ ] Include links to previous/next directory table blocks
 [ ] Change arrow-keys to next block / previous block
 [ ] More hotkeys? (d to jump to directory head?)(b for BAM ?)

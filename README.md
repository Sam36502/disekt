# Disekt

A disk-sector visualisation tool
Takes a C64 image file and displays the BAM (Block-Availability Map)
in a circular disk.

It also works for checking the integrity of a disk backed up using my
janky custom tool. This means it can show the quality of a sector's
information based on whether or not the checksums match and if
a certain sector is missing or not.

## File Format

Disk-Dump Reconciliation is done via a separate file (.r64)
that contains a 16-byte record for each sector of the disk.
The records have the following format:

	Pos.	Size	Description
	0x00	1B		Block Type; Can be one of the following
						Empty blocks:
							0x00 - Missing: This block hasn't been read (disk image is all 0x00s)
							0x01 - Empty: This block is blank, but this matches what we expect from the BAM
						Partial Data:
							0x11 - Corrupted: This block has data, but the checksums don't match
							0x12 - Unconfirmed: This block has data, but it doesn't match the directory contents
						Full / Healthy Data:
							0x20 - Confirmed: This block's data matches all expectations
							0x2x - Healthy: This block has been confirmed by `x` subsequent aggregations
						Metadata:
							0xFx - Metadata: This record contains something other than block info; See metadata section below
	0x01	1B		Track Number (1-indexed)
	0x02	1B		Sector Index (0-indexed)
	0x03	1B		Unused (reserved)
	0x04	2B		2-Byte Checksum received from the C64 directly
	0x06	10B		Unused
	
The recon file as a whole has a basic header that is as follows:

	Pos.	Size	Description
	0x00	4B		Magic bytes "D64R"
	0x04	4B		Offset (in bytes) from the start of the file to the first metadata/reconciliation record
	0x06	4B		Offset to the start of the arbitrary data region
	0x08	4B		Offset to the start of the metadata string region
	0x10 - Start of data

After that, the file is structured into 3 "regions"
	- Metadata/Reconciliation Records:
		These are the 16-byte records described above, which correspond to the sectors on the disk and report on their statuses
	- Arbitrary Data:
		This area is reserved for arbitrary data, which can be anything, but is intended to store incomplete or
		conflicting data blocks for certain disk segments, which allows the main disk file to only contain complete records.
		If someone felt like it though, they could do more such as embedding compressed pictures of the disk itself
	- Metadata Strings:
		This area lets you store basic disk metadata along with the disk image;
		It's intended for things like preserving the names or tables of contents or production dates where present

Each region SHOULD align to a 16-byte boundary

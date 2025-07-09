-- Script to convert a plain hex dump to nyblog format
--

-- Read input filename
local infile_name = arg[1]
if infile_name == nil then
	print("No input filename; exiting...\n")
	os.exit(1, true)
end

-- Open File
local infile = io.open(infile_name, "r")
if infile == nil then
	print("Failed to open file '"..infile_name.."'; exiting...\n")
	os.exit(1, true)
end

-- Read string
local text = infile:read("a")
infile:close()

-- Parse digits
local num_split = text:gmatch("%x%x%s")
local nums = {}
for m in num_split do
	nums[#nums+1] = tonumber(m, 16)
end

-- Read all blocks in order

local i_start = 1
while i_start < #nums do

	-- Find synch signal
	for i=i_start,#nums-1 do
		if (nums[i] == 0xFF and nums[i+1] == 0x00) then
			i_start = i+2
			break
		end
	end

	-- Print block header
	io.write(string.format(">>> BLOCK-START: T=%i; S=%i <<<\n", nums[i_start], nums[i_start + 1]))
	io.write("         00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n")
	i_start = i_start + 2

	-- Print Block data
	for y=0,16-1 do
		io.write(string.format("  [0x%02X]", y*16))
		for x=0,16-1 do
			local i = i_start + y*16 + x
			io.write(string.format(" %02X", nums[i]))
		end
		io.write("\n")
	end
	i_start = i_start + 256

	-- Print Footer
	io.write(string.format(">>> BLOCK-END; C=0x%02X%02X; E=%i <<<\n", nums[i_start + 1], nums[i_start], nums[i_start + 2]))
	if nums[i_start + 2] ~= 0x00 then
		io.write(string.format(">>> INFO; M='Block end doesn't match expected format (0x%02X)' <<<\n", nums[i_start + 2]))
	end
	i_start = i_start + 4

end


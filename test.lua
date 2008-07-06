require("bz2")

-- read in a file and display the uncompressed data to stdout (i.e. this
-- behaves identically to bzcat)
b = bz2.read_open("access_log.bz2")
function read_chunk() return bz2.read(b, 1024) end
text = read_chunk()
while text ~= nil do
	io.write(text)
	text = read_chunk()
end
bz2.read_close(b)

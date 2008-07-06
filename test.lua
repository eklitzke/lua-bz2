require("bz2")

-- read in a file and display the uncompressed data to stdout (i.e. this
-- behaves identically to bzcat)
b = bz2.read_open("access_log.bz2")
text = bz2.read(b, 1024)
while text ~= nil do
	io.write(text)
	text = bz2.read(b, 1024)
	if text == nil then
		break
	end
	io.write(text)
end
bz2.read_close(b)

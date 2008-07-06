require("bz2")

-- read in a file and display the uncompressed data to stdout (i.e. this
-- behaves identically to bzcat)
b = bz2.open("access_log.bz2")
text = b:read(1024)
while text ~= nil do
	io.write(text)
	text = b:read(1024)
end
b:close()

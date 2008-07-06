require("bz2")

b = bz2.read_open("access_log.bz2")
while 1 do
	text = bz2.read(b, 10)
	if text == nil then
		break
	end
	io.write(text)
end
bz2.read_close(b)

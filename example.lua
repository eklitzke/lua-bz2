-- This script prints all of the lines in the access line that start with 10.
-- (i.e. the IP address in the access log was a 10.x.x.x IP address)
require("bz2")

b = bz2.open("access_log.bz2")
line = b:getline()
while line ~= nil do
	if line:find("^10\.") then io.write(line) end
	line = b:getline()
end
b:close()

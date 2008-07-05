require("bz2")

b = bz2.read_open("access_log.bz2")
print(bz2.read(b, 10000))
bz2.read_close(b)

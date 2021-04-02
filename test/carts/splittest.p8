pico-8 cartridge // http://www.pico-8.com
version 32
__lua__
function _draw()
 cls()
 print("ex 1", 3)
	foreach (split("1,2,3"), print) -- returns {1,2,3}
 print("ex 2", 4)
 foreach (split("one:two:3",":",false), print) -- returns {"one","two","3"}
 print("ex 3", 5)
 foreach (split("1,,2,"), print) -- returns {1,"",2,""}
 print("ex 4", 7)
 foreach (split(nil), print)
 print(tostr(split(nil)))
 local nilsplit = split(nil)
 print(nilsplit)
 print(type(nilsplit))
end
__gfx__
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00700700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00077000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00077000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00700700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
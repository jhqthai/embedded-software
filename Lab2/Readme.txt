Fixes/Bugs TBA
Const type for constant that’s not part of switch case
{ after function
Check for boundary in write16/32
FSTAT -> W1C to clear

Lab2 Explanation

0x00080000  A
0x00080001  B

0x00080002  C
0x00080003  D

0x00080004  E
0x00080005  F

0x00080006  G
0x00080007  H

_______________________________________________________________________________

Flash8(0x00080001, "Z");
lets find the halfword containing 0x00080001

halfword 0x00080000 "AB"
|
---> "AZ"

Flash16(0x00080000, "AZ");
lets find the word containing 0x00080000

word 0x00080000 "ABCD"
|
---> "AZCD"

Flash32(0x00080000, "AZCD");


00 00 00 00

___________________________________________________________________________________________

Flash8(0x00080005, "X");
is this first or second byte of halfword?
second

what was the first byte? (0x00080005-1) E
our byte is at 0x00080005
create a halfword of value E + X
halfword starts at address 0x00080004

___________________________________________________________________________________________

Flash8(0x00080004, "X");
is this first or second byte of halfword?
first

what was the second byte? 0x00080004 + 1 F
our byte is at 0x00080004
create a halfword of value X + F
halfword starts at address 0x0080004






0x000800000100
00000000000001

00000000000000
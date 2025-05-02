# CRC8-Library

This is a library of mutually-compatible and verified C and Python procedures and associated test programs for computing the CRC-8 checksum of a stream of 8-bit bytes.

## Background

As of February, 2022, Github offers 45 C and 13 Python libraries for CRC-8 calculations.  So: Why did I write another one?

Well, if you're a stickler for details, read on. 

I needed a CRC-8 library that met several criteria:

- Simple, uncluttered, fast CRC-8 checksum calculation over a stream of 8-bit bytes: that means using table-lookups for the CRC division operation
- Verifiable results compared with recognized standards
- Demonstrably "the best" algorithm for the size of my byte stream (9 bytes = 72 bits, or 10 bytes=80 bits including the CRC)
- Compatible/interchangeable code for both C and Python: be able to checksum a message being sent from a system running in one language and receive and verify it on another system running in another language.

I spent several days reading papers and searching for programs that met my criteria.  I couldn't find one -- much less a C-Python pair.

So I adapted from a number of sources to create the C library I needed, then  wrote the equivalent as a Python module.  And I wrote programs that used those libraries to test against published standard results as a way of verifying both that the individual libraries functioned correctly and that they were compatible.

There are two seminal papers as the basis for these libraries:

- [http://ross.net/crc/download/crc_v3.txt](http://ross.net/crc/download/crc_v3.txt), an excellent paper by Ross William in 1993, that provides a detailed explanation of the CRC-8 algorithms and some example code, and
- [https://users.ece.cmu.edu/~koopman/roses/dsn04koopman04_crc_poly_embedded.pdf](https://users.ece.cmu.edu/~koopman/roses/dsn04/koopman04_crc_poly_embedded.pdf), which provides an exceptional analysis of the quality of the algorithm based upon choices of parameters and the size of the messages and the nature of transmission errors.

In particular, if you are looking for a CRC-8 library to compute checksums for messages of length > 119 bits or with known transmission error characteristics, please read Koopman's work before proceeding with any CRC-8 library -- an 8-bit CRC algorithm might not be the most effective for your use case, and Koopman may have good advice on the polynomial choice if the message size is small.

There are a number of programs that use the table-lookup CRC algorithm, but in most or all of them it is hard to discern:

- what is the divisor polynomial?
- is the message reflected?  Byte-wise or message-wise.
- is the polynomial reversed?
- what is the initial remainder, or is it 0x00 by default?
- is the result XORed with 0xFF or not?

After reading Koopman's papers, in particular, it was clear that I wanted flexibility in choosing my intial polynomial.  I also wanted to default to his "best" choice for divisor polynomial for messages with length <=119 (0x97, or the "C2" polynomial).  But I also wanted to be able to check my results against known standards.

So I wanted a library that would do the following:

-  Use lookup tables for CRC-8 calculations
-  Allow me to pick the divisor polynomial, but default to 0x97 
-  Allow me to select the "initial remainder"
-  _NOT_ XOR the result within the library

In the end, I needed test programs that would test the libraries in both C and Python in such a way that I could verify against a standard published source.  I found one reference source that seemed reputable, from AUTOSAR [I'd welcome suggestions of other standards if you know of any!].   So I wrote testing programs to verify the CRC-8 checksums from both the C and Python libraries against tables in section 7.2.1 of:
[https://www.autosar.org/fileadmin/user_upload/standards/classic/20-11/AUTOSAR_SWS_CRCLibrary.pdf](https://users.ece.cmu.edu/~koopman/roses/dsn04/koopman04_crc_poly_embedded.pdf)

# Use

The CRC-8 routines are simple to use:

- If you don't do anything else, 
> 	**crc8(msg,Length,init)**
	
	gives you the CRC-8 checksum of a stream of bytes in "msg", of length "Length", with initial remainder "init", 
	using the polynomial 0x97 as the divisor.  If you don't understand all of that, read William's paper.  
	But 0x97 is the "best" choice for messages of length <=119, and 0 would be a reasonable choice for "init".  
	(Notice, though, that the AUTOSAR tables use 0xFF for "init", which is also another common choice.) 
	
	"msg" is an array of uint8_t in c and a bytearray() in Python; "Length" is an integer; 
	and "init" is a uint8_t in C and an integer in Python.
- If you want to choose your own polynomial divisor,
> **buildCRC8Table(poly)**

	where "poly" is a uint8_t byte in C and an integer in Python.
- To find out what the current polynomial divisor is,
> **getCRC8poly()**

	returns that value (as a uint8_t in C and an integer in Python).
- To see the current CRC-8 remainder table, 
> **dumpCRC8Table()**

	prints the 256-byte table to the standard output.
	
If you're programming in C, you'll need to "#include libcrc8.h" in files that reference library procedures.  
If you're programming in Python, you'll need to "import libcrc8".

If you want to XOR the resulting CRC-8 checksum (as some algorithms do), you'll need to do it yourself in the calling program with the result from the crc8() function call.  If you want your message reflected, you'll have to do it before calling crc8.  Again, this is intended to be a simple, straightforward CRC-8 calculation with no complications hidden in the library.  If you find otherwise, please let me know.

## Testing

Both the C and Python libraries include testing programs that generate the CRC-8 tables in the AUTOSAR reference.  You may want to review those as working examples for your use.

## Author
Written by H David Todd, February, 2022, but not my original work: I copied example code from many other sources along the way.  

Please report errors or make comments to hdtodd@gmail.com.  Thanks!

David


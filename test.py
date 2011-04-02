#!/usr/bin/python

import regression as test

def opc_zipread_test(path):
	test.call(test.build("opc_zipread"), [], ["--verify", test.docs(path)], test.tmp(path+".opc_zipread"), [])
	test.regr(test.docs(path+".opc_zipread"), test.tmp(path+".opc_zipread"))

def opc_dump_test(path):
	test.call(test.build("opc_dump"), [], [test.docs(path)], test.tmp(path+".opc_dump"), [])
	test.regr(test.docs(path+".opc_dump"), test.tmp(path+".opc_dump"))

if __name__ == "__main__":
#	opc_zipread_test("simple.zip")
	opc_zipread_test("OOXMLI1.docx")
	opc_zipread_test("OOXMLI4.docx")

	opc_dump_test("OOXMLI1.docx")
	opc_dump_test("OOXMLI4.docx")

	test.rm(test.tmp("out.zip"))
#	text.exist("zip_write.zip")
#	text.exec("zip_dump", ["zip_write.zip", "zip_write.zip_dump"])

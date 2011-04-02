#!/usr/bin/python

import regression as test

def opc_zipread_test(path):
	test.call(test.build("opc_zipread"), [], ["--verify", test.docs(path)], test.tmp(path+".opc_zipread"), [])
	test.regr(test.docs(path+".opc_zipread"), test.tmp(path+".opc_zipread"), True)

def opc_dump_test(path):
	test.call(test.build("opc_dump"), [], [test.docs(path)], test.tmp(path+".opc_dump"), [])
	test.regr(test.docs(path+".opc_dump"), test.tmp(path+".opc_dump"), True)

def opc_extract_test(path, part):
	out_ext=".opc_extract."+part.replace("/", "-")
	test.call(test.build("opc_extract"), [], [test.docs(path), part], test.tmp(path)+out_ext, [])
	test.regr(test.docs(path)+out_ext, test.tmp(path)+out_ext, False)

def opc_mem_test(path):
	test.call(test.build("opc_mem"), [], [test.docs(path)], test.tmp(path+".opc_mem"), [])
	test.regr(test.docs(path+".opc_dump"), test.tmp(path+".opc_mem"), True)

def opc_type_test(path):
	test.call(test.build("opc_type"), [], [test.docs(path)], test.tmp(path+".opc_type"), [])
	test.regr(test.docs(path+".opc_type"), test.tmp(path+".opc_type"), True)
	
def opc_relation_test(path):
	test.call(test.build("opc_relation"), [], [test.docs(path)], test.tmp(path+".opc_relation"), [])
	test.regr(test.docs(path+".opc_relation"), test.tmp(path+".opc_relation"), True)
	test.call(test.build("opc_relation"), [], [test.docs(path), "rId1"], test.tmp(path+".opc_relation.rId1"), [])
	test.regr(test.docs(path+".opc_relation.rId1"), test.tmp(path+".opc_relation.rId1"), True)
	test.call(test.build("opc_relation"), [], [test.docs(path), "word/document.xml", "rId1"], test.tmp(path+".opc_relation.word-document.rId1"), [])
	test.regr(test.docs(path+".opc_relation.word-document.rId1"), test.tmp(path+".opc_relation.word-document.rId1"), True)
	


if __name__ == "__main__":
#	opc_zipread_test("simple.zip")
	opc_zipread_test("OOXMLI1.docx")
	opc_zipread_test("OOXMLI4.docx")

	opc_dump_test("OOXMLI1.docx")
	opc_dump_test("OOXMLI4.docx")

	opc_extract_test("OOXMLI1.docx", "word/document.xml")

	opc_mem_test("OOXMLI1.docx")
	opc_mem_test("OOXMLI4.docx")

	opc_type_test("OOXMLI1.docx")
	opc_type_test("OOXMLI4.docx")

	opc_relation_test("OOXMLI1.docx")
	opc_relation_test("OOXMLI4.docx")

	test.rm(test.tmp("out.zip"))
#	text.exist("zip_write.zip")
#	text.exec("zip_dump", ["zip_write.zip", "zip_write.zip_dump"])

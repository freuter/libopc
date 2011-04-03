#!/usr/bin/python

import regression as test
import os
import sys

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
	
def opc_image_test(path):
	test.call(test.build("opc_image"), [], [test.docs(path), test.tmp("")], test.tmp(path+".opc_image"), [])
	test.regr(test.docs(path+".opc_image"), test.tmp(path+".opc_image"), True)

def opc_text_test(path):
	test.call(test.build("opc_text"), [], [test.docs(path)], test.tmp(path+".opc_text.html"), [])
	test.regr(test.docs(path+".opc_text.html"), test.tmp(path+".opc_text.html"), True)

def opc_part_test(path):
	test.call(test.build("opc_part"), [], [test.docs(path), "word/document.xml"], test.tmp(path+".opc_part"), [])
	test.regr(test.docs(path+".opc_part"), test.tmp(path+".opc_part"), False)

def opc_xml_test(path):
	test.call(test.build("opc_xml"), [], [test.docs(path), "word/document.xml"], test.tmp(path+".opc_xml"), [])
	test.regr(test.docs(path+".opc_xml"), test.tmp(path+".opc_xml"), True)

def opc_xml2_test(path):
	test.call(test.build("opc_xml2"), [], [test.docs(path)], test.tmp(path+".opc_xml2"), [])
	test.regr(test.docs(path+".opc_xml2"), test.tmp(path+".opc_xml2"), True)

def opc_zipextract_test(path):
	test.call(test.build("opc_zipextract"), [], [test.docs(path), "word/document.xml"], test.tmp(path+".opc_zipextract"), [])
	test.regr(test.docs(path+".opc_zipextract"), test.tmp(path+".opc_zipextract"), True)

def opc_zipwrite_test(path):
	test.rm(test.tmp(path))
	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--import", "--delete", "--add", "--commit"], test.tmp("stdout.txt"), [])
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_1.opc_zipread"), [])
	test.regr(test.docs(path+"_1.opc_zipread"), test.tmp(path+"_1.opc_zipread"), True)

	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--import", "--commit"], test.tmp("stdout.txt"), [])
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_2.opc_zipread"), [])
	test.regr(test.tmp(path+"_1.opc_zipread"), test.tmp(path+"_2.opc_zipread"), True)

	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--import", "--trim"], test.tmp("stdout.txt"), [])
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_3.opc_zipread"), [])
	test.regr(test.docs(path+"_3.opc_zipread"), test.tmp(path+"_3.opc_zipread"), True)

	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--delete", "--commit"], test.tmp("stdout.txt"), [])
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_4.opc_zipread"), [])
	test.regr(test.docs(path+"_4.opc_zipread"), test.tmp(path+"_4.opc_zipread"), True)

	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--delete", "--trim"], test.tmp("stdout.txt"), [])
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_5.opc_zipread"), [])
	test.regr(test.docs(path+"_5.opc_zipread"), test.tmp(path+"_5.opc_zipread"), True)

if __name__ == "__main__":
	opc_zipwrite_test("out.zipwrite")
	sys.exit()

#	opc_zipread_test("simple.zip")
	opc_zipread_test("OOXMLI1.docx")
	opc_zipread_test("OOXMLI4.docx")

	opc_zipextract_test("OOXMLI1.docx")
	opc_zipextract_test("OOXMLI4.docx")

	opc_dump_test("OOXMLI1.docx")
	opc_dump_test("OOXMLI4.docx")

	opc_extract_test("OOXMLI1.docx", "word/document.xml")

	opc_mem_test("OOXMLI1.docx")
	opc_mem_test("OOXMLI4.docx")

	opc_type_test("OOXMLI1.docx")
	opc_type_test("OOXMLI4.docx")

	opc_relation_test("OOXMLI1.docx")
	opc_relation_test("OOXMLI4.docx")

	opc_image_test("OOXMLI1.docx")
	opc_image_test("OOXMLI4.docx")

	opc_text_test("OOXMLI1.docx")
	opc_text_test("OOXMLI4.docx")

	opc_part_test("OOXMLI1.docx")

	opc_xml_test("OOXMLI1.docx")

	opc_xml2_test("OOXMLI1.docx")
	opc_xml2_test("OOXMLI4.docx")

#	text.exist("zip_write.zip")
#	text.exec("zip_dump", ["zip_write.zip", "zip_write.zip_dump"])

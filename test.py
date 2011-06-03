#!/usr/bin/python

import regression as test
import os
import sys
import getopt

def opc_zipread_test(path):
	test.call(test.build("opc_zipread"), [], ["--verify", test.docs(path)], test.tmp(path+".opc_zipread"), [], {})
	test.regr(test.docs(path+".opc_zipread"), test.tmp(path+".opc_zipread"), True)

def opc_dump_test(path):
	test.call(test.build("opc_dump"), [], [test.docs(path)], test.tmp(path+".opc_dump"), [], {})
	test.regr(test.docs(path+".opc_dump"), test.tmp(path+".opc_dump"), True)

def opc_extract_test(path, part):
	out_ext=".opc_extract."+part.replace("/", "-")
	test.call(test.build("opc_extract"), [], [test.docs(path), part], test.tmp(path)+out_ext, [], {})
	test.regr(test.docs(path)+out_ext, test.tmp(path)+out_ext, False)

def opc_mem_test(path):
	test.call(test.build("opc_mem"), [], [test.docs(path)], test.tmp(path+".opc_mem"), [], {})
	test.regr(test.docs(path+".opc_dump"), test.tmp(path+".opc_mem"), True)

def opc_type_test(path):
	test.call(test.build("opc_type"), [], [test.docs(path)], test.tmp(path+".opc_type"), [], {})
	test.regr(test.docs(path+".opc_type"), test.tmp(path+".opc_type"), True)
	
def opc_relation_test(path):
	test.call(test.build("opc_relation"), [], [test.docs(path)], test.tmp(path+".opc_relation"), [], {})
	test.regr(test.docs(path+".opc_relation"), test.tmp(path+".opc_relation"), True)
	test.call(test.build("opc_relation"), [], [test.docs(path), "rId1"], test.tmp(path+".opc_relation.rId1"), [], {})
	test.regr(test.docs(path+".opc_relation.rId1"), test.tmp(path+".opc_relation.rId1"), True)
	test.call(test.build("opc_relation"), [], [test.docs(path), "word/document.xml", "rId1"], test.tmp(path+".opc_relation.word-document.rId1"), [], {})
	test.regr(test.docs(path+".opc_relation.word-document.rId1"), test.tmp(path+".opc_relation.word-document.rId1"), True)
	
def opc_image_test(path):
	test.call(test.build("opc_image"), [], [test.docs(path), test.tmp("")], test.tmp(path+".opc_image"), [], {})
	test.regr(test.docs(path+".opc_image"), test.tmp(path+".opc_image"), True)

def opc_text_test(path):
	test.call(test.build("opc_text"), [], [test.docs(path)], test.tmp(path+".opc_text.html"), [], {})
	test.regr(test.docs(path+".opc_text.html"), test.tmp(path+".opc_text.html"), True)

def opc_part_test(path):
	test.call(test.build("opc_part"), [], [test.docs(path), "word/document.xml"], test.tmp(path+".opc_part"), [], {})
	test.regr(test.docs(path+".opc_part"), test.tmp(path+".opc_part"), False)

def opc_xml_test(path):
	test.call(test.build("opc_xml"), [], [test.docs(path), "word/document.xml"], test.tmp(path+".opc_xml"), [], {})
	test.regr(test.docs(path+".opc_xml"), test.tmp(path+".opc_xml"), True)

def opc_xml2_test(path):
	test.call(test.build("opc_xml2"), [], [test.docs(path)], test.tmp(path+".opc_xml2"), [], {})
	test.regr(test.docs(path+".opc_xml2"), test.tmp(path+".opc_xml2"), True)

def opc_zipextract_test(path):
	test.call(test.build("opc_zipextract"), [], [test.docs(path), "word/document.xml"], test.tmp(path+".opc_zipextract"), [], {})
	test.regr(test.docs(path+".opc_zipextract"), test.tmp(path+".opc_zipextract"), True)

def opc_zipwrite_test(path):
	test.rm(test.tmp(path))
	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--import", "--delete", "--add", "--commit"], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_1.opc_zipread"), [], {})
	test.regr(test.docs(path+"_1.opc_zipread"), test.tmp(path+"_1.opc_zipread"), True)

	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--import", "--commit"], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_2.opc_zipread"), [], {})
	test.regr(test.tmp(path+"_1.opc_zipread"), test.tmp(path+"_2.opc_zipread"), True)

	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--import", "--trim"], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_3.opc_zipread"), [], {})
	test.regr(test.docs(path+"_3.opc_zipread"), test.tmp(path+"_3.opc_zipread"), True)

	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--delete", "--commit"], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_4.opc_zipread"), [], {})
	test.regr(test.docs(path+"_4.opc_zipread"), test.tmp(path+"_4.opc_zipread"), True)

	test.call(test.build("opc_zipwrite"), [], [test.tmp(path), "--delete", "--trim"], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_zipread"), [], ["--verify", test.tmp(path)], test.tmp(path+"_5.opc_zipread"), [], {})
	test.regr(test.docs(path+"_5.opc_zipread"), test.tmp(path+"_5.opc_zipread"), True)

def opc_trim_test(path):
	test.rm(test.tmp(path))
	test.cp(test.docs(path), test.tmp(path))
	test.call(test.build("opc_dump"), [], [test.docs(path)], test.tmp(path+"_1.opc_trim.opc_dump"), [], {})
	test.regr(test.docs(path+".opc_dump"), test.tmp(path+"_1.opc_trim.opc_dump"), True)
	test.call(test.build("opc_trim"), [], [test.tmp(path)], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_dump"), [], [test.docs(path)], test.tmp(path+"_2.opc_trim.opc_dump"), [], {})
	test.regr(test.tmp(path+"_1.opc_trim.opc_dump"), test.tmp(path+"_2.opc_trim.opc_dump"), True)
	test.call(test.build("opc_trim"), [], [test.tmp(path)], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_dump"), [], [test.docs(path)], test.tmp(path+"_3.opc_trim.opc_dump"), [], {})
	test.regr(test.tmp(path+"_2.opc_trim.opc_dump"), test.tmp(path+"_3.opc_trim.opc_dump"), True)
	test.call(test.build("opc_trim"), [], [test.tmp(path)], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_dump"), [], [test.docs(path)], test.tmp(path+"_4.opc_trim.opc_dump"), [], {})
	test.regr(test.tmp(path+"_3.opc_trim.opc_dump"), test.tmp(path+"_4.opc_trim.opc_dump"), True)

def mce_extract_test(path, part, namespaces, returncode):
	_part_="."+part.replace('.', '_')
	args=[test.docs(path), part]
	for namespace in namespaces:
		args.append("--understands"); args.append(namespace[1])
		_part_=_part_+"."+namespace[0]
	test.call(test.build("mce_extract"), [], args, test.tmp(path+_part_+".mce_extract"), [], {"return": returncode})
	test.regr(test.docs(path+_part_+".mce_extract"), test.tmp(path+_part_+".mce_extract"), True)

def mce_write_test(path):
    test.rm(test.tmp(path))
    test.call(test.build("mce_write"), [], [test.tmp(path)], test.tmp("stdout.txt"), [], {})
    test.call(test.build("mce_extract"), [], [test.tmp(path), "sample.xml"], test.tmp(path+".mce_write.mce_extract"), [], {})
    test.regr(test.docs(path+".mce_write.mce_extract"), test.tmp(path+".mce_write.mce_extract"), False)
    test.call(test.build("mce_extract"), [], [test.tmp(path), "--understands", "http://schemas.openxmlformats.org/Circles/v2", "sample.xml"], test.tmp(path+".mce_write.v2.mce_extract"), [], {})
    test.regr(test.docs(path+".mce_write.v2.mce_extract"), test.tmp(path+".mce_write.v2.mce_extract"), False)


def opc_generate_test(basepath, path):
	dest=os.path.join(os.path.split(path)[0], os.path.splitext(os.path.split(path)[1])[0]+".c")
	test.ensureDir(test.tmp(os.path.dirname(dest)))
	test.call(test.build("opc_dump"), [], [os.path.join(basepath, path)], test.tmp(path+"_1.opc_generate.opc_dump"), [], {})
	test.call(test.build("opc_generate"), [], [os.path.join(basepath, path), test.tmp(dest)], test.tmp("stdout.txt"), [], {})
	exe=test.compile(test.tmp(dest))
	test.call(exe, [], [test.tmp(path)], test.tmp("stdout.txt"), [], {})
	test.call(test.build("opc_dump"), [], [test.tmp(path)], test.tmp(path+"_2.opc_generate.opc_dump"), [], {})
	test.regr(test.tmp(path+"_1.opc_generate.opc_dump"), test.tmp(path+"_2.opc_generate.opc_dump"), False)

def usage():
	print("usage:")
	print(" test [--target=mode \"debug|release\"]")
	print("      [--generate \"test_docs/\"] [--skip]")

if __name__ == "__main__":
	target_mode=None
	generate_path=None
	generate_skip=False
	try:
		opts, args = getopt.getopt(sys.argv[1:], "h", ["help", "target=", "generate=", "skip"])
	except getopt.GetoptError, err:
		# print help information and exit:
		print str(err) # will print something like "option -a not recognized"
		usage()
		sys.exit(2)
	for o, a in opts:
		if o in ("-h", "--help"):
			usage()
			sys.exit(0)
		elif o in ("--target"):
			target_mode=a
		elif o in ("--generate"):
			generate_path=a
		elif o in ("--skip"):
			generate_skip=True
	if None==target_mode and "nt"==os.name:
		print "please specify target mode, e.g. --target=debug"
		sys.exit(2)
	else:
		test.init(target_mode)
#	sys.exit(0)

	if None==generate_path:

#		sys.exit(0)

		opc_zipread_test("stream.zip") # fails --- streaming mode is not yet implemented.
		opc_zipread_test("OOXMLI1.docx")
		opc_zipread_test("OOXMLI4.docx")

		opc_zipextract_test("OOXMLI1.docx")
		opc_zipextract_test("OOXMLI4.docx")

		opc_zipwrite_test("out.zipwrite")

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

		opc_trim_test("OOXMLI1.docx")
		opc_trim_test("OOXMLI4.docx")

		mce_extract_test("mce.zip", "circles-ignorable.xml", [], 0)
		mce_extract_test("mce.zip", "circles-ignorable.xml", [["v2", "http://schemas.openxmlformats.org/Circles/v2"]], 0)
		mce_extract_test("mce.zip", "circles-ignorable.xml", [["v2", "http://schemas.openxmlformats.org/Circles/v2"], ["v3", "http://schemas.openxmlformats.org/Circles/v3"]], 0)

		mce_extract_test("mce.zip", "circles-ignorable-ns.xml", [], 0)
		mce_extract_test("mce.zip", "circles-ignorable-ns.xml", [["v1", "http://schemas.openxmlformats.org/MyExtension/v1"]], 0)

		mce_extract_test("mce.zip", "circles-plugin.xml", [], 0)
		mce_extract_test("mce.zip", "circles-plugin.xml", [["v2", "http://schemas.openxmlformats.org/Circles/v2"]], 0)

		mce_extract_test("mce.zip", "circles-alternatecontent.xml", [], 0)
		mce_extract_test("mce.zip", "circles-alternatecontent.xml", [["v2", "http://schemas.openxmlformats.org/Circles/v2"]], 0)
		mce_extract_test("mce.zip", "circles-alternatecontent.xml", [["v2", "http://schemas.openxmlformats.org/Circles/v2"], ["v3", "http://schemas.openxmlformats.org/Circles/v3"]], 0)

		mce_extract_test("mce.zip", "circles-alternatecontent2.xml", [], 0)
		mce_extract_test("mce.zip", "circles-alternatecontent2.xml", [["v1", "http://schemas.openxmlformats.org/metallicfinishes/v1"]], 0)

		mce_extract_test("mce.zip", "circles-processcontent.xml", [], 0)
		mce_extract_test("mce.zip", "circles-processcontent.xml", [["v2", "http://schemas.openxmlformats.org/Circles/v2"]], 0)

		mce_extract_test("mce.zip", "circles-processcontent-ns.xml", [], 0)
		mce_extract_test("mce.zip", "circles-processcontent-ns.xml", [["ext", "http://schemas.openxmlformats.org/Circles/extension"]], 0)

		mce_extract_test("mce.zip", "circles-mustunderstand.xml", [], 2)
		mce_extract_test("mce.zip", "circles-mustunderstand.xml", [["v2", "http://schemas.openxmlformats.org/Circles/v2"]], 0)

		mce_write_test("mce_write.zip")

	else:
		ignore_list = {  }
		skip_list = {  }
		ignore_file = os.path.join(generate_path, "opc_generate.ignore")
		skip_file = os.path.join(generate_path, "opc_generate.skip")
		if os.path.exists(ignore_file):
			f=open(ignore_file, "r")
			ignore_list=eval(f.read())
			f.close()
#		print "ignore_list="+str(ignore_list)
		if os.path.exists(skip_file):
			f=open(skip_file, "r")
			for line in f.readlines():
				line=line.strip()
				if ""!=line:
					skip_list[line]=True
			f.close()
#		print "skip_list="+str(skip_list)
		skip_f=open(skip_file, "w")
		for item in skip_list:
			skip_f.write(item+"\n")
		skip_f.flush()
			
		suitename=os.path.split(generate_path)[1]
		basedir=os.path.split(generate_path)[0]
		for root, dirs, files in os.walk(generate_path):
			for name in files:
				src=os.path.relpath(os.path.join(root, name), basedir)
				f=os.path.relpath(src, suitename).replace(os.sep, "/")
				dst=os.path.join(suitename, f)
				ext=os.path.splitext(src)[1]
				ignore=f in ignore_list and ignore_list[f]
				if generate_skip:
					skip=f in skip_list and skip_list[f]
				else:
					skip=False
#				print f+" "+str(ignore)
				if not(ignore) and not(skip) and (".docx"==ext or ".xlsx"==ext or ".pptx"==ext or ".xps"==ext):
					print "opc_generate_test("+basedir+", "+src+")"
					opc_generate_test(basedir, src)
					skip_f.write(f+"\n")
					skip_f.flush()
		skip_f.close()
#		opc_generate_test(test.docs(""), "helloworld.pptx")
#		opc_generate_test(test.docs(""), "OOXMLI1.docx")
	#	opc_generate_test(test.docs(""), "OOXMLI4.docx") ## this is toooo much for the compiler: "fatal error C1060: compiler is out of heap space"
